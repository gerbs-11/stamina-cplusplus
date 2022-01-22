#ifndef STAMINA
#define STAMINA

#define DEBUG_PRINTS

#include <iostream>
#include <ostream>
#include <functional>

#include "StaminaArgParse.h"
#include "StaminaModelChecker.h"
#include "Options.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 1

#include <storm/api/storm.h>
#include <storm-parsers/api/storm-parsers.h>
#include <storm-parsers/parser/PrismParser.h>
#include <storm/storage/prism/Program.h>
#include <storm/storage/jani/Property.h>
#include <storm/modelchecker/results/CheckResult.h>
#include <storm/modelchecker/results/ExplicitQuantitativeCheckResult.h>

// #include <storm/utility/initialize.h>

namespace stamina {
    /* ERRORS WE CAN GET */
    enum STAMINA_ERRORS {
        ERR_GENERAL = 1
        , ERR_SEVERE = 2
        , ERR_MEMORY_EXCEEDED = 137
    };
    /* MAIN STAMINA CLASS */
    class Stamina {
    public:
        /**
         * Main construtor: creates an instance of the STAMINA class
         * 
         * @param arguments Arguments struct from StaminaArgParse
         * */
        Stamina(struct arguments * arguments);
        /**
         * Destructor. Cleans up memory
         * */
        ~Stamina();
        /**
         * Runs stamina
         * */
        void run();
        /* Data Members */
        /**
         * Initializes Stamina
         * */
        void initialize();

        /* Data Members */
        StaminaModelChecker * modelChecker;
        storm::prism::Program modulesFile;
        std::vector<storm::jani::Property> propertiesVector;
    };
}

#endif // STAMINA
