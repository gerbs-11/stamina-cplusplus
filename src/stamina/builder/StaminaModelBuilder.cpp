/**
* Implementation for StaminaModelBuilder methods.
*
* Created by Josh Jeppson on 8/17/2021
* */
#include "StaminaModelBuilder.h"
#include "../StaminaMessages.h"
#include "../StateSpaceInformation.h"

#include <functional>
#include <sstream>

namespace stamina {
namespace builder {

template <typename ValueType, typename RewardModelType, typename StateType>
StaminaModelBuilder<ValueType, RewardModelType, StateType>::StaminaModelBuilder(
	std::shared_ptr<storm::generator::PrismNextStateGenerator<ValueType, StateType>> const& generator
	, storm::prism::Program const& modulesFile
	, storm::generator::NextStateGeneratorOptions const & options
) : generator(generator)
	, stateStorage(*(new storm::storage::sparse::StateStorage<StateType>(generator->getStateSize())))
	, absorbingWasSetUp(false)
	, fresh(true)
	, firstIteration(true)
	, localKappa(Options::kappa)
	, numberTerminal(0)
	, iteration(0)
	, propertyExpression(nullptr)
	, formulaMatchesExpression(true)
	, stateRemapping(std::vector<uint_fast64_t>())
	, modulesFile(modulesFile)
	, options(options)
	, terminalStateToIdCallback(
		std::bind(
			&StaminaModelBuilder<ValueType, RewardModelType, StateType>::getStateIndexOrAbsorbing
			, this
			, std::placeholders::_1
		)
	)
{
}

template <typename ValueType, typename RewardModelType, typename StateType>
StaminaModelBuilder<ValueType, RewardModelType, StateType>::StaminaModelBuilder(
	storm::prism::Program const& program
	, storm::generator::NextStateGeneratorOptions const& generatorOptions
) : StaminaModelBuilder( // Invoke other constructor
	std::make_shared<storm::generator::PrismNextStateGenerator<ValueType, StateType>>(program, generatorOptions)
	, program
	, generatorOptions
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
				isCtmc = true;
				return storm::utility::builder::buildModelFromComponents(storm::models::ModelType::Ctmc, buildModelComponents());
			case storm::generator::ModelType::DTMC:
				isCtmc = false;
				StaminaMessages::warning("This model is a DTMC. If you are using this in the STAMINA program, currently, only CTMCs are supported. You may get an error in checking.");
				return storm::utility::builder::buildModelFromComponents(storm::models::ModelType::Dtmc, buildModelComponents());
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
	return nullptr;
}

template <typename ValueType, typename RewardModelType, typename StateType>
std::vector<StateType>
StaminaModelBuilder<ValueType, RewardModelType, StateType>::getPerimeterStates() {
	std::vector<StateType> perimeterStates = stateMap.getPerimeterStates();

	return perimeterStates;
}

template <typename ValueType, typename RewardModelType, typename StateType>
StateType
StaminaModelBuilder<ValueType, RewardModelType, StateType>::getOrAddStateIndex(CompressedState const& state) {
	StateType actualIndex;
	StateType newIndex = static_cast<StateType>(stateStorage.getNumberOfStates());
	if (stateStorage.stateToId.contains(state)) {
		actualIndex = stateStorage.stateToId.getValue(state);
	}
	else {
		// Create new index just in case we need it
		actualIndex = newIndex;
	}

	auto nextState = stateMap.get(actualIndex);

	stateStorage.stateToId.findOrAdd(state, actualIndex);

	return actualIndex;
}

template <typename ValueType, typename RewardModelType, typename StateType>
StateType
StaminaModelBuilder<ValueType, RewardModelType, StateType>::getStateIndexOrAbsorbing(CompressedState const& state) {
	if (stateStorage.stateToId.contains(state)) {
		return stateStorage.stateToId.getValue(state);
	}
	// This state should not exist yet and should point to the absorbing state
	return 0;
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::flushToTransitionMatrix(storm::storage::SparseMatrixBuilder<ValueType>& transitionMatrixBuilder) {
	for (StateType row = 0; row < transitionsToAdd.size(); ++row) {
		if (transitionsToAdd[row].empty()) {
			// This state is deadlock
			transitionMatrixBuilder.addNextValue(row, row, 1);
		}
		else {
			for (TransitionInfo tInfo : transitionsToAdd[row]) {
				transitionMatrixBuilder.addNextValue(row, tInfo.to, tInfo.transition);
			}
		}
	}
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::createTransition(StateType from, StateType to, ValueType probability) {
	TransitionInfo tInfo(to, probability);
	// Create an element for both from and to
	while (transitionsToAdd.size() <= std::max(from, to)) {
		transitionsToAdd.push_back(std::vector<TransitionInfo>());
	}
	transitionsToAdd[from].push_back(tInfo);
}


template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::remapStates(storm::storage::SparseMatrixBuilder<ValueType>& transitionMatrixBuilder) {

	// State Remapping
	std::vector<uint_fast64_t> const& remapping = stateRemapping.get();

	if (remapping.size() < numberStates) {
		StaminaMessages::warning(
			std::string("Remapping vector and number of explored states do not match sizes!")
			+ "\n\tVector Size: " + std::to_string(remapping.size())
			+ "\n\tNumber of states: " + std::to_string(numberStates)
		);
	}
	else {
		StaminaMessages::info(
			std::string("Remapping vector and number of explored states match.")
			+ "\n\tVector Size: " + std::to_string(remapping.size())
			+ "\n\tNumber of states: " + std::to_string(numberStates)
		);
	}

	// According to the STORM Folks, this is what needs to be done
	// in order to use the state remapping:

	// Fix the transition matrix with the new entries
	transitionMatrixBuilder.replaceColumns(remapping, 0);

    // Fix the initial state indecies
    // (not sure if we need to do this since our initial state indecies are fine)
	std::vector<StateType> newInitialStateIndices(this->stateStorage.initialStateIndices.size());
	std::transform(
		this->stateStorage.initialStateIndices.begin()
		, this->stateStorage.initialStateIndices.end()
		, newInitialStateIndices.begin()
		, [&remapping](StateType const& state) {
			return remapping[state];
		}
	);
	std::sort(newInitialStateIndices.begin(), newInitialStateIndices.end());
	this->stateStorage.initialStateIndices = std::move(newInitialStateIndices);

	// Remap stateStorage.stateToId
	this->stateStorage.stateToId.remap(
		[&remapping](StateType const& state) {
			return remapping[state];
		}
	);

	this->generator->remapStateIds(
		[&remapping](StateType const& state) {
			return remapping[state];
		}
	);
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::printStateSpaceInformation() {
	StaminaMessages::info("Finished state space truncation.\n\tExplored " + std::to_string(numberStates) + " states in total.\n\tGot " + std::to_string(numberTransitions) + " transitions.");
}



template <typename ValueType, typename RewardModelType, typename StateType>
storm::models::sparse::StateLabeling
StaminaModelBuilder<ValueType, RewardModelType, StateType>::buildStateLabeling() {
	return generator->label(stateStorage, stateStorage.initialStateIndices, stateStorage.deadlockStateIndices);
}

template <typename ValueType, typename RewardModelType, typename StateType>
double
StaminaModelBuilder<ValueType, RewardModelType, StateType>::accumulateProbabilities() {
	double totalProbability = numberTerminal * localKappa;
	// Reduce kappa
	localKappa /= Options::reduce_kappa;
	return totalProbability;
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::setUpAbsorbingState(
	storm::storage::SparseMatrixBuilder<ValueType>& transitionMatrixBuilder
	, std::vector<RewardModelBuilder<typename RewardModelType::ValueType>>& rewardModelBuilders
	, StateAndChoiceInformationBuilder& choiceInformationBuilder
	, boost::optional<storm::storage::BitVector>& markovianChoices
	, boost::optional<storm::storage::sparse::StateValuationsBuilder>& stateValuationsBuilder
) {
	if (absorbingWasSetUp) {
		return;
	}
	if (firstIteration) {
		stateRemapping.get().push_back(storm::utility::zero<StateType>());
		this->absorbingState = CompressedState(generator->getVariableInformation().getTotalBitOffset(true)); // CompressedState(64);
		bool gotVar = false;
		for (auto variable : generator->getVariableInformation().booleanVariables) {
			if (variable.getName() == "Absorbing") {
				this->absorbingState.setFromInt(variable.bitOffset + 1, 1, 1);
				if (this->absorbingState.getAsInt(variable.bitOffset + 1, 1) != 1) {
					StaminaMessages::errorAndExit("Absorbing state setup failed!");
				}
				gotVar = true;
				break;
			}
		}
		if (!gotVar) {
			StaminaMessages::errorAndExit("Did not get \"Absorbing\" variable!");
		}
		// Add index 0 to deadlockstateindecies because the absorbing state is in deadlock
		stateStorage.deadlockStateIndices.push_back(0);
		// Check if state is already registered
		std::pair<StateType, std::size_t> actualIndexPair = stateStorage.stateToId.findOrAddAndGetBucket(absorbingState, 0);

		StateType actualIndex = actualIndexPair.first;
		if (actualIndex != 0) {
			StaminaMessages::errorAndExit("Absorbing state should be index 0! Got " + std::to_string(actualIndex));
		}
		absorbingWasSetUp = true;
		// transitionMatrixBuilder.addNextValue(0, 0, storm::utility::one<ValueType>());
		// This state shall be Markovian (to not introduce Zeno behavior)
		if (choiceInformationBuilder.isBuildMarkovianStates()) {
			choiceInformationBuilder.addMarkovianState(0);
		}
	}
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::reset() {
	if (fresh) {
		return;
	}
	statesToExplore.clear(); // = StatePriorityQueue();
	// exploredStates.clear(); // States explored in our current iteration
	// API reset
	// if (stateRemapping) { stateRemapping->clear(); }
	// stateStorage = storm::storage::sparse::StateStorage<StateType>(generator->getStateSize());
	absorbingWasSetUp = false;
}


template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::setGenerator(
	std::shared_ptr<storm::generator::PrismNextStateGenerator<ValueType, StateType>> generator
) {
	this->generator = generator;
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::setLocalKappaToGlobal() {
	Options::kappa = localKappa;
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::connectTerminalStatesToAbsorbing(
	storm::storage::SparseMatrixBuilder<ValueType>& transitionMatrixBuilder
	, CompressedState & terminalState
	, StateType stateId
	, std::function<StateType (CompressedState const&)> stateToIdCallback
) {
	bool addedValue = false;
	generator->load(terminalState);
	storm::generator::StateBehavior<ValueType, StateType> behavior = generator->expand(stateToIdCallback);
	// If there is no behavior, we have an error.
	if (behavior.empty()) {
		StaminaMessages::warning("Behavior for perimeter state was empty!");
		return;
	}
	for (auto const& choice : behavior) {
		double totalRateToAbsorbing = 0;
		for (auto const& stateProbabilityPair : choice) {
			if (stateProbabilityPair.first != 0) {
				// row, column, value
				createTransition(stateId, stateProbabilityPair.first, stateProbabilityPair.second);
			}
			else {
				totalRateToAbsorbing += stateProbabilityPair.second;
			}
		}
		addedValue = true;
		// Absorbing state
		createTransition(stateId, 0, totalRateToAbsorbing);
	}
	if (!addedValue) {
		StaminaMessages::errorAndExit("Did not add to transition matrix!");
	}
}

template <typename ValueType, typename RewardModelType, typename StateType>
storm::expressions::Expression *
StaminaModelBuilder<ValueType, RewardModelType, StateType>::getPropertyExpression() {
	return propertyExpression;
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::setPropertyFormula(
	std::shared_ptr<const storm::logic::Formula> formula
	, const storm::prism::Program & modulesFile
) {
	formulaMatchesExpression = false;
	propertyFormula = formula;
	this->expressionManager = &modulesFile.getManager();
}

template <typename ValueType, typename RewardModelType, typename StateType>
void
StaminaModelBuilder<ValueType, RewardModelType, StateType>::loadPropertyExpressionFromFormula() {
	if (formulaMatchesExpression) {
		return;
	}
	// If we are called here, we assume that Options::no_prop_refine is false
	std::shared_ptr<storm::expressions::Expression> pExpression(
		// Invoke copy constructor
		new storm::expressions::Expression(
			propertyFormula->toExpression(
				// Expression manager
				*(this->expressionManager)
			)
		)
	);
	formulaMatchesExpression = true;
}


// Explicitly instantiate the class.
template class StaminaModelBuilder<double, storm::models::sparse::StandardRewardModel<double>, uint32_t>;

template <typename StateType>
bool set_contains(std::unordered_set<StateType> current_set, StateType value) {
	auto search = current_set.find(value);
	return (search != current_set.end());
}

} // namespace builder
} // namespace stamina
