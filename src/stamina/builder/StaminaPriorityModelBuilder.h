#ifndef STAMINA_PRIORITY_MODEL_BUILDER_H
#define STAMINA_PRIORITY_MODEL_BUILDER_H

/**
 * The model builder class which implements the STAMINA 3.0 algorithm which uses a priority queue
 * on the estimated reachability
 *
 * Created by Josh Jeppson on Jun 9, 2021
 * */

#include "StaminaModelBuilder.h"

namespace stamina {
	namespace builder {
		template<typename ValueType, typename RewardModelType = storm::models::sparse::StandardRewardModel<ValueType>, typename StateType = uint32_t>
		class StaminaPriorityModelBuilder : public StaminaModelBuilder<ValueType, RewardModelType, StateType> {
		public:
			typedef typename StaminaModelBuilder<ValueType, RewardModelType, StateType>::ProbabilityState ProbabilityState;
			/**
			* Constructs a StaminaPriorityModelBuilder with a given storm::generator::PrismNextStateGenerator. Invokes super's constructor
			*
			* @param generator The generator we are going to use.
			* */
			StaminaPriorityModelBuilder(
				std::shared_ptr<storm::generator::PrismNextStateGenerator<ValueType, StateType>> const& generator
				, storm::prism::Program const& modulesFile
				, storm::generator::NextStateGeneratorOptions const & options
			);
			/**
			* Constructs a StaminaPriorityModelBuilder with a PRISM program and generatorOptions. Invokes super's constructor
			*
			* @param program The PRISM program we are going to use to build the model with.
			* @param generatorOptions Options for the storm::generator::PrismNextStateGenerator we are going to use.
			* */
			StaminaPriorityModelBuilder(
				storm::prism::Program const& program
				, storm::generator::NextStateGeneratorOptions const& generatorOptions = storm::generator::NextStateGeneratorOptions()
			);
			/**
			* Gets the state ID of a current state, or adds it to the internal state storage. Performs state exploration
			* and state space truncation from that state.
			*
			* @param state Pointer to the state we are looking it
			* @return A pair with the state id and whether or not it was already discovered
			* */
			StateType getOrAddStateIndex(CompressedState const& state) override;
			/**
			* Explores state space and truncates the model
			*
			* @return The components of the truncated model
			* */
			storm::storage::sparse::ModelComponents<ValueType, RewardModelType> buildModelComponents() override;
			/**
			* Builds transition matrix of truncated state space for the given program.
			*
			* @param transitionMatrixBuilder The builder of the transition matrix.
			* @param rewardModelBuilders The builders for the selected reward models.
			* @param choiceInformationBuilder The builder for the requested information of the choices
			* @param markovianChoices is set to a bit vector storing whether a choice is Markovian (is only set if the model type requires this information).
			* @param stateValuationsBuilder if not boost::none, we insert valuations for the corresponding states
			* */
			void buildMatrices(
				storm::storage::SparseMatrixBuilder<ValueType>& transitionMatrixBuilder
				, std::vector<RewardModelBuilder<typename RewardModelType::ValueType>>& rewardModelBuilders
				, StateAndChoiceInformationBuilder& choiceInformationBuilder
				, boost::optional<storm::storage::BitVector>& markovianChoices
				, boost::optional<storm::storage::sparse::StateValuationsBuilder>& stateValuationsBuilder
			) override;
			/*
			 * Access to data members of parent class
			 * */
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::propertyExpression;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::expressionManager;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::propertyFormula;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::stateStorage;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::generator;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::memoryPool;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::statesToExplore;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::stateRemapping;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::stateMap;
			// Options for next state generators
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::options;
			// The model builder must have access to this to create a fresh next state generator each iteration
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::modulesFile;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::currentProbabilityState;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::absorbingState;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::absorbingWasSetUp;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::isInit;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::fresh;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::iteration;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::firstIteration;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::localKappa;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::isCtmc;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::formulaMatchesExpression;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::numberTerminal;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::numberStates;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::numberTransitions;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::currentRowGroup;
			using StaminaModelBuilder<ValueType, RewardModelType, StateType>::currentRow;
		private:
			/**
			 * Connects all states which are terminal
			 * */
			void connectAllTerminalStatesToAbsorbing(storm::storage::SparseMatrixBuilder<ValueType>& transitionMatrixBuilder);
			/* Data members */
			std::priority_queue<
				typename StaminaModelBuilder<ValueType, RewardModelType, StateType>::ProbabilityStatePair
				, typename StaminaModelBuilder<ValueType, RewardModelType, StateType>::ProbabilityStatePair
				, typename StaminaModelBuilder<ValueType, RewardModelType, StateType>::ProbabilityStatePairComparison
			> statePriorityQueue;
			uint64_t numberOfExploredStates;
			uint64_t numberOfExploredStatesSinceLastMessage;
			double piHat;
		};
	}
}
#endif // STAMINA_PRIORITY_MODEL_BUILDER_H
