#include "StaminaMessages.h"

#include "ANSIColors.h"

#include <stdlib.h>
#include <iomanip>
#include <stdlib.h>
#include <string_view>
#include <sstream>
#include <iostream>

using namespace stamina;

void
StaminaMessages::errorAndExit(std::string err, uint8_t err_num) {
	std::cerr << BOLD(FRED("[ERROR]: "));
	std::cerr << BOLD("STAMINA encountered the following error and will now exit: ") << std::endl;
	std::cerr << '\t' << err << std::endl;
	exit(err_num);
}

void
StaminaMessages::error(std::string err, uint8_t err_num) {
	std::cerr << BOLD(FRED("[ERROR]: "));
	std::cerr << BOLD("STAMINA encountered the following (possibly recoverable) error: ") << std::endl;
	std::cerr << '\t' << err << std::endl;
}

void
StaminaMessages::warning(std::string warn) {
	std::cerr << BOLD(FYEL("[WARNING]: ")) << warn << std::endl;
}

void
StaminaMessages::info(std::string info) {
	std::cerr << BOLD(FBLU("[INFO]: ")) << info << std::endl;
}

void
StaminaMessages::good(std::string good) {
	std::cerr << BOLD(FGRN("[MESSAGE]: ")) << good << std::endl;
}

#ifdef DEBUG_PRINTS
void StaminaMessages::debugPrint(std::string msg) {
	std::cout << BOLD(FMAG("[DEBUG MESSAGE]: ")) << msg << std::endl;
}
#endif

void
StaminaMessages::writeResults(ResultInformation resultInformation, std::ostream out) {
	out.setf( std:: ios::floatfield );
	out << std::fixed << std::setprecision(12);
	out << horizontalSeparator << std::endl;
	out << "RESULTS" << std::endl << horizontalSeparator << std::endl;
	out << "Property: " << resultInformation.property << std::endl;
	out << "Probability Minimum: " << resultInformation.pMin << std::endl;
	out << "Probability Maximum: " << resultInformation.pMax << std::endl;
	out << "Window: " << (resultInformation.pMax - resultInformation.pMin) << std::endl;
	out << horizontalSeparator << std::endl;
	out << "Model: " << resultInformation.numberStates << " states with " << resultInformation.numberInitial << " initial." << std::endl;
	out << horizontalSeparator << std::endl;
}
