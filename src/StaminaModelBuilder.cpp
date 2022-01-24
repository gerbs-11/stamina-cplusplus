/**
* Implementation for StaminaModelBuilder methods.
*
* Created by Josh Jeppson on 8/17/2021
* */
#include "StaminaModelBuilder.h"
#include "StaminaMessages.h"

#define DEBUG_PRINTS


#include <functional>
#include <sstream>

#include "storm/builder/RewardModelBuilder.h"
#include "storm/builder/StateAndChoiceInformationBuilder.h"

#include "storm/exceptions/AbortException.h"
#include "storm/exceptions/WrongFormatException.h"
#include "storm/exceptions/IllegalArgumentException.h"

#include "storm/generator/PrismNextStateGenerator.h"
#include "storm/generator/JaniNextStateGenerator.h"

#include "storm/models/sparse/Dtmc.h"
#include "storm/models/sparse/Ctmc.h"
#include "storm/models/sparse/Mdp.h"
#include "storm/models/sparse/MarkovAutomaton.h"
#include "storm/models/sparse/StandardRewardModel.h"

#include "storm/settings/modules/BuildSettings.h"

#include "storm/storage/expressions/ExpressionManager.h"
#include "storm/storage/jani/Model.h"
#include "storm/storage/jani/Automaton.h"
#include "storm/storage/jani/AutomatonComposition.h"
#include "storm/storage/jani/ParallelComposition.h"

#include "storm/utility/builder.h"
#include "storm/utility/constants.h"
#include "storm/utility/prism.h"
#include "storm/utility/macros.h"
#include "storm/utility/ConstantsComparator.h"
#include "storm/utility/SignalHandler.h"

using namespace stamina;

template <typename ValueType, typename RewardModelType, typename StateType>
StaminaModelBuilder<ValueType, RewardModelType, StateType>::StaminaModelBuilder(
	std::shared_ptr<storm::generator::PrismNextStateGenerator<ValueType, StateType>> const& generator
) : generator(generator)
	, stateStorage(generator->getStateSize())
{
	// Intentionally left empty
}

template <typename ValueType, typename RewardModelType, typename StateType>
StaminaModelBuilder<ValueType, RewardModelType, StateType>::StaminaModelBuilder(
	storm::prism::Program const& program
	, storm::generator::NextStateGeneratorOptions const& generatorOptions
) : StaminaModelBuilder( // Invoke other constructor
	std::make_shared<storm::generator::PrismNextStateGenerator<ValueType, StateType>>(program, generatorOptions)
)
{
	// Intentionally left empty
}

template <typename ValueType, typename RewardModelType, typename StateType>
StaminaModelBuilder<ValueType, RewardModelType, StateType>::StaminaModelBuilder(
	storm::jani::Model const& model
	, storm::generator::NextStateGeneratorOptions const& generatorOptions
) : StaminaModelBuilder( // Invoke other constructor
	std::make_shared<storm::generator::PrismNextStateGenerator<ValueType, StateType>>(model, generatorOptions)
)
{
	// Intentionally left empty
}

template <typename ValueType, typename RewardModelType, typename StateType>
std::shared_ptr<storm::models::sparse::Model<ValueType, RewardModelType>>
StaminaModelBuilder<ValueType, RewardModelType, StateType>::build() {
	try {
		switch (generator->getModelType()) {
			// Only supports CTMC models.
			case storm::generator::ModelType::CTMC:
				return storm::utility::builder::buildModelFromComponents(storm::models::ModelType::Ctmc, buildModelComponents());
			case storm::generator::ModelType::DTMC:
			case storm::generator::ModelType::MDP:
			case storm::generator::ModelType::POMDP:
			case storm::generator::ModelType::MA:
			default:
				StaminaMessages::error("This model type is not supported!");
		}
		return nullptr;
	}
	catch(const std::exception& e) {
		std::stringstream ss;
		ss << "STAMINA encountered the following error (possibly in the interface with STORM)";
		ss << " in the function StaminaModelBuilder::build():\n\t" << e.what();
		StaminaMessages::error(ss.str());
	}

}

template <typename ValueType, typename RewardModelType, typename StateType>
bool
StaminaModelBuilder<ValueType, RewardModelType, StateType>::shouldEnqueue(StateType previousState) {
	// If our previous state has not been encountered, we have unexpected behavior
	if (piMap.find(previousState) == piMap.end()) {
		piMap.insert({previousState, (float) 0.0});
		// Show ERROR that unexpected behavior has been encountered (we've reached a state we shouldn't have been able to)
		StaminaMessages::error("Unexpected behavior! State with index " + std::to_string(previousState)
			+ " should have already been in the probability map, but it was not! Inserting now."
			+ "\nThis indicates that we have (somehow) reached a state that did not show up in "
			+ "any previous states' next state list."
		);
		return false;
	}
	// If we haven't reached this state before, insert it into piMap
	if (piMap.find(currentState) == piMap.end()) {
		piMap.insert({currentState, (float) 0.0});
	}
	// If the reachability probability of the previous state is 0, enqueue regardless
	if (piMap[previousState] == 0.0) {
		return true;
	}
	// Otherwise, we base it on whether the maps we keep track of contain them
	return !(set_contains(stateMap, currentState) && set_contains(exploredStates, currentState));
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::updateReachabilityProbability(
	StateType currentState
	, StateType previousState
	, float transitionProbability
) {
	// Optimization to prevent unnecessary multiply
	if (piMap[previousState] == 0.0) {
		return;
	}
	// If we haven't reached this state before, insert it into piMap
	if (piMap.find(currentState) == piMap.end()) {
		piMap.insert({currentState, (float) 0.0});
	}
	// Update the probability
	piMap[currentState] += transitionProbability * piMap[previousState];
}

template <typename ValueType, typename RewardModelType, typename StateType>
StateType
StaminaModelBuilder<ValueType, RewardModelType, StateType>::getOrAddStateIndex(CompressedState const& state) {
	// Create new index just in case we need it
	StateType newIndex = static_cast<StateType>(stateStorage.getNumberOfStates());

	// Check if state is already registered
	std::pair<StateType, std::size_t> actualIndexPair = stateStorage.stateToId.findOrAddAndGetBucket(state, newIndex);

	StateType actualIndex = actualIndexPair.first;

	// If this method is getting called, we must enqueue the state
	// Determines if we need to insert the state
	if (actualIndex == newIndex && shouldEnqueue(actualIndex)) {
		// Always does breadth first search
		statesToExplore.emplace_back(state, actualIndex);
	}

	return actualIndex;
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::buildMatrices(
	storm::storage::SparseMatrixBuilder<ValueType>& transitionMatrixBuilder
	, std::vector<RewardModelBuilder<typename RewardModelType::ValueType>>& rewardModelBuilders
	, StateAndChoiceInformationBuilder& stateAndChoiceInformationBuilder
	, boost::optional<storm::storage::BitVector>& markovianChoices
	, boost::optional<storm::storage::sparse::StateValuationsBuilder>& stateValuationsBuilder
) {

	// Builds model
	// Initialize building state valuations (if necessary)
	if (stateAndChoiceInformationBuilder.isBuildStateValuations()) {
		stateAndChoiceInformationBuilder.stateValuationsBuilder() = generator->initializeStateValuationsBuilder();
	}

	// Create a callback for the next-state generator to enable it to request the index of states.
	std::function<StateType (CompressedState const&)> stateToIdCallback = std::bind(
		&StaminaModelBuilder<ValueType, RewardModelType, StateType>::getOrAddStateIndex
		, this
		, std::placeholders::_1
	);

	// Let the generator create all initial states.
	this->stateStorage.initialStateIndices = generator->getInitialStates(stateToIdCallback);
	if (!this->stateStorage.initialStateIndices.empty()) {
		StaminaMessages::error("Initial states are empty!");
	}

	// Now explore the current state until there is no more reachable state.
	uint_fast64_t currentRowGroup = 0;
	uint_fast64_t currentRow = 0;

	auto timeOfStart = std::chrono::high_resolution_clock::now();
	auto timeOfLastMessage = std::chrono::high_resolution_clock::now();
	uint64_t numberOfExploredStates = 0;
	uint64_t numberOfExploredStatesSinceLastMessage = 0;

	StateType currentIndex;

	// Perform a search through the model.
	while (!statesToExplore.empty()) {
		// Get the first state in the queue.
		CompressedState currentState = statesToExplore.front().first;
		currentIndex = statesToExplore.front().second;
		// Set our state variable in the class
		// NOTE: this->currentState is not the same as CompressedState currentState
		this->currentState = currentIndex;
		statesToExplore.pop_front();

		if (currentIndex % 100000 == 0) {
			StaminaMessages::info("Exploring state with id " + std::to_string(currentIndex) + ".");
		}

		// Add state to piMap if it is not in there
		if (piMap.find(currentIndex) == piMap.end()) {
			piMap.insert({currentIndex, 0.0});
		}

		// Do not explore if state is terminal and its reachability probability is less than kappa
		if (setContains(tSet, currentIndex) && piMap[currentIndex] < Options::kappa) {
			continue;
		}

		// We assume that if we make it here, our state is either nonterminal, or its reachability probability
		// is greater than kappa

		// Load state for us to use
		generator->load(currentState);
		if (stateAndChoiceInformationBuilder.isBuildStateValuations()) {
			generator->addStateValuation(currentIndex, stateAndChoiceInformationBuilder.stateValuationsBuilder());
		}
		storm::generator::StateBehavior<ValueType, StateType> behavior = generator->expand(stateToIdCallback);

		// If there is no behavior, we have an error.
		if (behavior.empty()) {
			StaminaMessages::errorExit("Behavior for state " + std::to_string(currentIndex) + " was empty!");
		}
		else {
			// Determine whether or not to enqueue all next states
			bool shouldEnqueueAll = piMap[currentIndex] == 0.0;

			if (!shouldEnqueueAll && setContains(tMap, currentIndex)) {
				// Remove currentIndex from T if it's in T
				tMap.remove(currentIndex);
			}

			// Add the state rewards to the corresponding reward models.
			auto stateRewardIt = behavior.getStateRewards().begin();
			for (auto& rewardModelBuilder : rewardModelBuilders) {
				if (rewardModelBuilder.hasStateRewards()) {
					rewardModelBuilder.addStateReward(*stateRewardIt);
				}
				++stateRewardIt;
			}

			// Now add all choices.
			bool firstChoiceOfState = true;
			for (auto const& choice : behavior) {

				// add the generated choice information
				if (stateAndChoiceInformationBuilder.isBuildChoiceLabels() && choice.hasLabels()) {
					for (auto const& label : choice.getLabels()) {
						stateAndChoiceInformationBuilder.addChoiceLabel(label, currentRow);
					}
				}
				if (stateAndChoiceInformationBuilder.isBuildChoiceOrigins() && choice.hasOriginData()) {
					stateAndChoiceInformationBuilder.addChoiceOriginData(choice.getOriginData(), currentRow);
				}
				if (stateAndChoiceInformationBuilder.isBuildStatePlayerIndications() && choice.hasPlayerIndex()) {
					if (firstChoiceOfState) {
						stateAndChoiceInformationBuilder.addStatePlayerIndication(choice.getPlayerIndex(), currentRowGroup);
					}
				}

				// Add the probabilistic behavior to the matrix.
				for (auto const& stateProbabilityPair : choice) {
					StateType sPrime = stateProbabilityPair.first;
					float probability = stateProbabilityPair.second;
					// Insert sPrime into piMap if not there already
					if (piMap.find(sPrime) == piMap.end()) {
						piMap.insert({sPrime, 0.0});
					}
					// Update transition probability
					piMap[sPrime] += piMap[currentIndex] * probability;
					if (!(setContains(stateMap, sPrime) && setContains(exploredStates, sPrime))) {
						// Add s' to ExploredStates
						exploredStates.insert(sPrime);
						// Enqueue S is handled in stateToIdCallback
					}
					transitionMatrixBuilder.addNextValue(currentRow, sPrime, probability);
				}

				++currentRow;
				firstChoiceOfState = false;
			}
			// Set our current state's reachability probability to 0
			piMap[currentIndex] = 0;
			++currentRowGroup;
		}

		++numberOfExploredStates;
		if (generator->getOptions().isShowProgressSet()) {
			++numberOfExploredStatesSinceLastMessage;

			auto now = std::chrono::high_resolution_clock::now();
			auto durationSinceLastMessage = std::chrono::duration_cast<std::chrono::seconds>(now - timeOfLastMessage).count();
			if (static_cast<uint64_t>(durationSinceLastMessage) >= generator->getOptions().getShowProgressDelay()) {
				auto statesPerSecond = numberOfExploredStatesSinceLastMessage / durationSinceLastMessage;
				auto durationSinceStart = std::chrono::duration_cast<std::chrono::seconds>(now - timeOfStart).count();
				StaminaMessages::info(
					"Explored " << numberOfExploredStates << " states in " << durationSinceStart << " seconds (currently " << statesPerSecond << " states per second)."
				);
				timeOfLastMessage = std::chrono::high_resolution_clock::now();
				numberOfExploredStatesSinceLastMessage = 0;
			}
		}

	}
}

template <typename ValueType, typename RewardModelType, typename StateType>
storm::storage::sparse::ModelComponents<ValueType, RewardModelType>
StaminaModelBuilder<ValueType, RewardModelType, StateType>::buildModelComponents() {
	// Is this model deterministic? (I.e., is there only one choice per state?)
	bool deterministic = generator->isDeterministicModel();

	// Component builders
	storm::storage::SparseMatrixBuilder<ValueType> transitionMatrixBuilder(0, 0, 0, false, !deterministic, 0);
	std::vector<RewardModelBuilder<typename RewardModelType::ValueType>> rewardModelBuilders;
	// Iterate through the reward models and add them to the rewardmodelbuilders
	for (uint64_t i = 0; i < generator->getNumberOfRewardModels(); ++i) {
		rewardModelBuilders.emplace_back(generator->getRewardModelInformation(i));
	}

	// Build choice information and markovian states
	storm::builder::StateAndChoiceInformationBuilder stateAndChoiceInformationBuilder;
	boost::optional<storm::storage::BitVector> markovianStates;

	// Build state valuations if necessary. We may not need this since we operate only on CTMC
	boost::optional<storm::storage::sparse::StateValuationsBuilder> stateValuationsBuilder;
	if (generator->getOptions().isBuildStateValuationsSet()) {
		stateValuationsBuilder = generator->initializeStateValuationsBuilder();
	}

	// Builds matrices and truncates state space
	buildMatrices(
		transitionMatrixBuilder
		, rewardModelBuilders
		, stateAndChoiceInformationBuilder
		, markovianStates
		, stateValuationsBuilder
	);

	// Using the information from buildMatrices, initialize the model components
	storm::storage::sparse::ModelComponents<ValueType, RewardModelType> modelComponents(
		transitionMatrixBuilder.build(0, transitionMatrixBuilder.getCurrentRowGroupCount())
		, buildStateLabeling()
		, std::unordered_map<std::string, RewardModelType>()
		, !generator->isDiscreteTimeModel()
		, std::move(markovianStates)
	);

	// Build choice labeling
	modelComponents.choiceLabeling = stateAndChoiceInformationBuilder.buildChoiceLabeling(modelComponents.transitionMatrix.getRowCount());
	if (generator->getOptions().isBuildChoiceOriginsSet()) {
		auto originData = stateAndChoiceInformationBuilder.buildDataOfChoiceOrigins(modelComponents.transitionMatrix.getRowCount());
		modelComponents.choiceOrigins = generator->generateChoiceOrigins(originData);
	}
	if (generator->isPartiallyObservable()) {
		std::vector<uint32_t> classes;
		classes.resize(stateStorage.getNumberOfStates());
		std::unordered_map<uint32_t, std::vector<std::pair<std::vector<std::string>, uint32_t>>> observationActions;
		for (auto const& bitVectorIndexPair : stateStorage.stateToId) {
			uint32_t varObservation = generator->observabilityClass(bitVectorIndexPair.first);
			classes[bitVectorIndexPair.second] = varObservation;
		}

		modelComponents.observabilityClasses = classes;
		if(generator->getOptions().isBuildObservationValuationsSet()) {
			modelComponents.observationValuations = generator->makeObservationValuation();
		}
	}
	return modelComponents;
}

template <typename ValueType, typename RewardModelType, typename StateType>
storm::models::sparse::StateLabeling
StaminaModelBuilder<ValueType, RewardModelType, StateType>::buildStateLabeling() {
	return generator->label(stateStorage, stateStorage.initialStateIndices, stateStorage.deadlockStateIndices);
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::setReachabilityThreshold(double threshold) {
	reachabilityThreshold = threshold;
}


// Explicitly instantiate the class.
template class StaminaModelBuilder<double, storm::models::sparse::StandardRewardModel<double>, uint32_t>;

// STAMINA DOES NOT USE ANY OF THESE FORWARD DEFINITIONS
// #ifdef STORM_HAVE_CARL
// template class StaminaModelBuilder<storm::RationalNumber, storm::models::sparse::StandardRewardModel<storm::RationalNumber>, uint32_t>;
// template class StaminaModelBuilder<storm::RationalFunction, storm::models::sparse::StandardRewardModel<storm::RationalFunction>, uint32_t>;
// template class StaminaModelBuilder<double, storm::models::sparse::StandardRewardModel<storm::Interval>, uint32_t>;
// #endif

template <typename StateType>
bool stamina::set_contains(std::unordered_set<StateType> current_set, StateType value) {
	auto search = current_set.find(value);
	return (search != current_set.end());
}
