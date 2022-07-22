/**
 * Base class for all types of threads used by Stamina's model builders
 *
 * Created by Josh Jeppson on Jul 8, 2022
 * */

#ifndef STAMINA_BUILDER_THREADS_BASETHREAD_H
#define STAMINA_BUILDER_THREADS_BASETHREAD_H

#include <thread>
#include <shared_mutex>

#include "stamina/builder/__storm_needed_for_builder.h"

namespace stamina {
	namespace builder {
		// Forward-declare StaminaModelBuilder class
		template<typename ValueType, typename RewardModelType = storm::models::sparse::StandardRewardModel<ValueType>, typename StateType = uint32_t>
		class StaminaModelBuilder;

		namespace threads {
			using namespace storm::builder;
			/**
			* Base class for all threads. Automatically constructs a thread which runs
			* the mainLoop function.
			* */
			template <typename StateType, typename RewardModelType, typename ValueType>
			class BaseThread {
			public:
				/**
				* Constructs a BaseThread
				*
				* @param parent The model builder who owns this thread
				* */
				BaseThread(StaminaModelBuilder<ValueType, RewardModelType, StateType> * parent);
				/**
				* Pure virtual function for the main loop. When this function returns,
				* the thread dies.
				* */
				virtual void mainLoop() = 0;
				/**
				* Creates and starts this thread in the background
				* */
				void startThread();
				/**
				* Gets the pointer to the model builder owning this thread
				*
				* @return This thread's parent
				* */
				const StaminaModelBuilder<ValueType, RewardModelType, StateType> * getParent();
				/**
				 * Joins the
				 * */
				void join();
				void terminate();
				void setHold(bool hold);
				bool isHolding();
			protected:
				bool finished;
				bool hold; // Should we continue idling even if finished?
				const StaminaModelBuilder<ValueType, RewardModelType, StateType> * parent;
			private:
				std::thread * threadLoop;
			};
		} // namespace threads
	} // namespace builder
} // namespace stamina


#endif // STAMINA_BUILDER_THREADS_BASETHREAD_H