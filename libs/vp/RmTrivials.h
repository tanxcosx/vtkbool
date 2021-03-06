/*
Copyright 2012-2018 Ronald Römer

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef __RmTrivials_h
#define __RmTrivials_h

#include <vector>
#include <iostream>
#include <memory>

#include "Tools.h"
#include "VisPoly.h"

class Vert2 {
public:
    Vert2 (int _i, double *_r, double _d) : i(_i), d(_d) {
        r[0] = _r[0];
        r[1] = _r[1];
    }
    int i;
    double r[2], d;

    friend std::ostream& operator<< (std::ostream &out, const Vert2 &v) {
        out << "i: " << v.i
            << ", d: " << v.d;
        return out;
    }

    bool operator< (const Vert2 &v) {
        return d < v.d;
    }
};

typedef std::vector<Vert2> VertsType2;

enum class Src {
    NONE = 1,
    A = 2,
    B = 3
};

class Vert3 : public Point {
public:
    Vert3 (Point &p) : Point(p), rm(false), src(Src::NONE), t(0) {}
    Vert3 (double *_s, Src _src, double _t) : Point(_s), rm(false), src(_src), t(_t) {}

    double t;
    Src src;
    bool rm;

    int i;

    friend std::ostream& operator<< (std::ostream &out, const Vert3 &v) {
        out << "id: " << v.id
            << ", pt: [" << v.x << "," << v.y << "]"
            << ", t: " << v.t
            << ", src: " << static_cast<int>(v.src);
        return out;
    }

};

typedef std::vector<Vert3> VertsType3;

enum class Dir {
    FORWARD = 1,
    BACKWARD = 2,
    UNDEFINED = 3
};

enum class Side {
    NONE = 1,
    IN = 2,
    OUT = 3
};

class Pair2 {
public:
    Pair2 (int _i, int _j, Dir _dir = Dir::FORWARD) : i(_i), j(_j), dir(_dir), side(Side::NONE) {}

    int i, j;
    IdsType pocket;
    Dir dir;
    Side side;

    friend std::ostream& operator<< (std::ostream &out, const Pair2 &p) {
        out << "i: " << p.i
            << ", j: " << p.j
            << ", dir: " << static_cast<int>(p.dir)
            << ", side: " << static_cast<int>(p.side)
            << ", pocket: [";

        for (int id : p.pocket) {
            out << id << ", ";
        }

        out << "]";
        return out;
    }

};

class Grp {
public:
    Grp (Dir _dir, IdsType _ids) : dir(_dir), ids(_ids) {}
    Dir dir;
    IdsType ids;
};

// ...

class Marked {
public:
    Marked (int _a, int _b) : a(_a), b(_b) {}

    int a, b;
    double t;

    friend std::ostream& operator<< (std::ostream &out, const Marked &m) {
        out << "[" << m.a << ", " << m.b << "] -> " << m.t;
        return out;
    }

};

class Vert4 : public Point {
public:
    Vert4 (Point &p) : Point(p) {}
    std::shared_ptr<Marked> marked;
};

typedef std::vector<Vert4> VertsType4;

void MarkInternals (VertsType4 &verts, int skip);

void RemoveInternals (VertsType4 &verts);

void AlignPts (VertsType4 &verts, int ind);

class Vert5 : public Point {
public:
    Vert5 (Point &p, double _t) : Point(p), t(_t), valid(true) {}

    double t;
    bool valid;

    bool operator< (const Vert5 &v) const {
        return t < v.t;
    }
};

typedef std::vector<Vert5> VertsType5;

void AddInternals (PolyType &origin, PolyType &poly);

// ...

class TrivialRm {
    PolyType &poly;
    VertsType3 verts;

    int ind;

    Point x;

public:
    TrivialRm (PolyType &_poly, int _ind) : poly(_poly), ind(_ind), x(_poly[_ind]) {}

    void GetSimplified (PolyType &res);

private:
    void GetPocket (Pair2 &pair, IdsType &pocket);
    void AssignSide (Pair2 &pair, Src src);
    bool HasArea (const IdsType &pocket);

    void RemovePockets (VertsType3 &good, double *rot, double d, Src src);
};

#endif
