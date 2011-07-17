struct StraightLine
{
    hugin_utils::FDiff2D start, mid, end;
    bool startSet, midSet, endSet;
    std::vector<hugin_utils::FDiff2D> points;
    StraightLine(): startSet(false), midSet(false), endSet(false) { } // constructor: falsify booleans
};
