// SPDX-License-Identifier: MIT
#pragma once
namespace lab {
// Require consecutive clean preparation passes: asynchronous reads may only
// be admitted on one pass and uploaded on a later one.
class ResourcePreparation {
    unsigned cleanFrames=0;
public:
    void restart(){cleanFrames=0;}
    void observe(unsigned waiting,bool failed){cleanFrames=waiting||failed?0:cleanFrames<2?cleanFrames+1:2;}
    bool pending()const{return cleanFrames<2;}
};
}
