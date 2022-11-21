#include "wayver-util.hpp"

using namespace Wayver;

const std::string Utility::floatArrayToString(float *arr, int len)
{

    std::string retval;
    for (int i = 0; i < len; i++){
        retval += ", " + std::to_string( arr[i] );
    }

    return retval;
}

const std::string Utility::floatVecToString( std::vector<float> vec)
{

    std::string retval;
    for (int i = 0; i < vec.size(); i++){
        retval += ", " + std::to_string(vec[i]);
    }

    return retval;
}