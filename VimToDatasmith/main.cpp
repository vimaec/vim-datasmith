//
//  main.cpp
//  VimToDatasmith
//
//  Created by Richard Young on 2021-05-17.
//

#include "VimToDatasmith.h"

#if macOS

#import <Foundation/Foundation.h>

int main(int argc, const char * argv[]) {
	int result = EXIT_FAILURE;
	@autoreleasepool {
		result = Vim2Ds::Convert(argc, argv);
	}
	return result;
}

#elif Windows


DISABLE_SDK_WARNINGS_START
#include "StringConv.h"
DISABLE_SDK_WARNINGS_END


int main(int argc, wchar_t* argv[], wchar_t* envp[]) {
	std::vector<Vim2Ds::utf8_string>	parameters;
	for (int i = 0; i < argc; ++i)
		parameters.push_back(argv[i]);
	std::vector<const Vim2Ds::utf8_t*>	paramArray;
	for (int i = 0; i < argc; ++i)
		paramArray.push_back(parameters[i].c_str());
	paramArray.push_back(nullptr);
	return Vim2Ds::Convert(argc, &paramArray[0]);
}


#endif
