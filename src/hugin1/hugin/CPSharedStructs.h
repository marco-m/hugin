struct StraightLine
{
    hugin_utils::FDiff2D start, mid, end;
    bool startSet, midSet, endSet;
    StraightLine(): startSet(false), midSet(false), endSet(false) { } // constructor
};
