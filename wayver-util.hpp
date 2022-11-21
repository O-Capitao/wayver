#pragma once

#include <string>
#include <vector>

namespace Wayver{
    namespace Utility
    {   

        const std::string floatArrayToString(float *arr, int len);
        
        const std::string floatVecToString( std::vector<float> vec);

    }    
}