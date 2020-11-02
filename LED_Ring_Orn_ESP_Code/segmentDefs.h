//segment definitions for the mask, see segmentSet.h in the library for explination of segments
//refer to pixel Layout excell doc for pixel numbering
//============================================================================================
//main rings, going clockwise

segmentSection ringSec0[] = {{0, 24}}; //outer ring 1, 24 pixels
Segment ringSegment0 = { SIZE(ringSec0), ringSec0, true }; //numSections, section array pointer

segmentSection ringSec1[] = {{24, 16}}; //outer ring 2, 16 pixels
Segment ringSegment1 = { SIZE(ringSec1), ringSec1, true }; //numSections, section array pointer

segmentSection ringSec2[] = {{40, 12}}; //outer ring 3, 12 pixels
Segment ringSegment2 = { SIZE(ringSec2), ringSec2, true }; //numSections, section array pointer

segmentSection ringSec3[] = {{52, 8}}; //outer ring 4, 8 pixels
Segment ringSegment3 = { SIZE(ringSec3), ringSec3, true }; //numSections, section array pointer

segmentSection ringSec4[] = {{60, 1}}; //outer ring 5, 1 pixel
Segment ringSegment4 = { SIZE(ringSec4), ringSec4, true }; //numSections, section array pointer

Segment *rings_arr[] = { &ringSegment0 , &ringSegment1, &ringSegment2, &ringSegment3, &ringSegment4 };
SegmentSet ringSegments = { SIZE(rings_arr), rings_arr };

//===========================================================================================
//rings split in halves, each set of halves belonging to a segement
//so we have series of groups, each belonging to one of two segements
segmentSection halfSec0[] = { {0, 12}, {24, 8}, {40, 6}, {52, 4}, {60, 1} };
Segment halfSegment0 = { SIZE(halfSec0), halfSec0, true }; //numSections, section array pointer

segmentSection halfSec1[] = { {12, 12}, {32, 8}, {46, 6}, {56, 4}, {60, 1} };
Segment halfSegment1 = { SIZE(halfSec1), halfSec1, true }; //numSections, section array pointer

Segment *half_arr[] = { &halfSegment0 , &halfSegment1 };
SegmentSet halfSegments = { SIZE(half_arr), half_arr };

//==================================================================================================================

//const PROGMEM segmentSection starSec0[] = {{0, 1}, {3, 1}, {6, 1}, {9, 1}, {12, 1}, {15, 1}, {18, 1}, {21, 1}};
//Segment starSegment0 = { SIZE(starSec0), starSec0, true }; //numSections, section array pointer
//
//const PROGMEM segmentSection starSec1[] = {{24, 1}, {26, 1}, {28, 1}, {30, 1}, {32, 1}, {34, 1}, {36, 1}, {38, 1}};
//Segment starSegment1 = { SIZE(starSec1), starSec1, true }; //numSections, section array pointer
//
//const PROGMEM segmentSection starSec2[] = {{40, 1}, {43, 1}, {46, 1}, {49, 1}};
//Segment starSegment2 = { SIZE(starSec2), starSec2, true }; //numSections, section array pointer
//
//const PROGMEM segmentSection starSec3[] = {{52, 8}};
//Segment starSegment3 = { SIZE(starSec3), starSec3, true }; //numSections, section array pointer
//
//const PROGMEM segmentSection starSec4[] = {{60, 1}};
//Segment starSegment4 = { SIZE(starSec4), starSec4, true }; //numSections, section array pointer
//
//Segment *stars_arr[] = { &starSegment0 , &starSegment1, &starSegment2, &starSegment3, &starSegment4 };
//SegmentSet starSegments = { SIZE(stars_arr), stars_arr };

//=======================================================================
//rings without vertical and horizonal lines to form a flower type pattern

segmentSection flowerSec0[] = {{1, 5}, {7, 5}, {13, 5}, {19, 5}};
Segment flowerSegment0 = { SIZE(flowerSec0), flowerSec0, true }; //numSections, section array pointer

segmentSection flowerSec1[] = {{25, 3}, {29, 3}, {33, 3}, {37, 3}};
Segment flowerSegment1 = { SIZE(flowerSec1), flowerSec1, true }; //numSections, section array pointer

segmentSection flowerSec2[] = {{41, 2}, {44, 2}, {47, 2}, {50, 2}};
Segment flowerSegment2 = { SIZE(flowerSec2), flowerSec2, true }; //numSections, section array pointer

segmentSection flowerSec3[] = {{53, 1}, {55, 1}, {57, 1}, {59, 1}};
Segment flowerSegment3 = { SIZE(flowerSec3), flowerSec3, true }; //numSections, section array pointer

segmentSection flowerSec4[] = {{60, 1}};
Segment flowerSegment4 = { SIZE(flowerSec4), flowerSec4, true }; //numSections, section array pointer

Segment *flower_arr[] = { &flowerSegment0 , &flowerSegment1, &flowerSegment2, &flowerSegment3, &flowerSegment4 };
SegmentSet flowerSegments = { SIZE(flower_arr), flower_arr };

//=============================================================================================
