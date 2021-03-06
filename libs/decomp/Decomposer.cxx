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

#include <cassert>
#include <algorithm>
#include <tuple>
#include <numeric>

#include "VisPoly.h"
#include "RmTrivials.h"

#include "Decomposer.h"

void SubP::AddPair (Pair _p, int _w) {
    if (_w > w) {
        return;
    }

    if (_w < w) {
        S.clear();
        S_tail.clear();
    }

    if (!S.empty()) {
        if (_p.f > S[0].f) {
            while (!S.empty() && S[0].g >= _p.g) {
                // das neue pair ist im intervall S[0] enthalten
                S.pop_front();
            }
        }
    }

    // durch einfügen an erster stelle ist S cw-ordered (gegenüber dem polygon)
    S.insert(S.begin(), _p);
    w = _w;

    S_head.clear();

}

void SubP::RestoreS () {
    S.insert(S.begin(), S_head.begin(), S_head.end());
    S.insert(S.end(), S_tail.rbegin(), S_tail.rend());

    S_head.clear();
    S_tail.clear();
}

Decomposer::Decomposer (PolyType &_poly) : poly(_poly) {
    {
        int i = 0;
        for(auto& p : poly) {
            assert(p.id == i++);
        }
    }

    Scale(poly);

    std::copy(poly.begin(), poly.end(), std::back_inserter(verts));

    SimpleRmInternals(verts);

    num = verts.size();

    for (int i = 0; i < num; i++) {
        int j = (i+1)%num,
            k = (i+num-1)%num;

        verts[i].refl = IsRefl(i, j, k);
    }

    auto first = std::find_if(verts.begin(), verts.end(), [&](const Vert6 &v) {
        return v.refl;
    });

    std::rotate(verts.begin(), first, verts.end());

    PolyType poly_(verts.begin(), verts.end());

    /*for (auto& p : poly_) {
        std::cout << p << std::endl;
    }*/

    for (int i = 0; i < num; i++) {
        if (verts[i].refl) {
            PolyType vp;
            GetVisPoly_wrapper(poly_, vp, i);

            for (auto& v : vp) {
                std::cout << v << std::endl;
            }

            PolyType::const_iterator itr;
            for (itr = vp.begin()+1; itr != vp.end(); ++itr) {
                if (itr->id == NO_USE) {
                    continue;
                }

                // die ids sind rel. bzgl. verts

                int a = vp[0].id,
                    b = itr->id;

                if (a < b) {
                    pairs.insert({a, b});
                } else {
                    pairs.insert({b, a});
                }

            }
        }
    }

    for (auto& v : verts) {
        std::cout << v << std::endl;
    }

    for (auto& p : pairs) {
        std::cout << p << std::endl;
    }

    // init um die reflexe

    for (int i = 0; i < num; i++) {
        if (verts[i].refl) {
            for (int j : {-2, -1, 1, 2}) {
                int k = i+j;

                if (k > 0 && k < num) {
                    int a, b;

                    if (i < k) {
                        a = i;
                        b = k;
                    } else {
                        a = k;
                        b = i;
                    }

                    std::cout << "> (" << a << ", " << b << ")" << std::endl;

                    SubP s;
                    s.w = 0;

                    if (b-a == 1) {
                        std::cout << "edge" << std::endl;
                    } else {
                        int c = a+1;
                        std::cout << "wedge" << std::endl;

                        s.S.push_back({c, c});
                    }

                    subs[{a, b}] = std::move(s);

                }

            }
        }
    }

}

bool Decomposer::IsRefl (int a, int b, int c) {
    //std::cout << "IsRefl " << a << ", " << b << ", " << c << std::endl;

    return IsNear(verts[b].pt, verts[c].pt) || (Ld(verts[a].pt, verts[b].pt, verts[c].pt) > 5e-3
        && Cross(verts[a].pt, verts[b].pt, verts[c].pt) < 0);
}

void Decomposer::Forw (int i, int j, int k) {
    //std::cout << "Forw " << i << " " << j << " " << k << std::endl;

    Pair p(i, j);

    //if (std::find(pairs.begin(), pairs.end(), p) == pairs.end()) {
    if (pairs.count(p) == 0) {
        return;
    }

    int a = j;
    int w = subs[p].w;

    if (k-j > 1) {
        Pair p_(j, k);
        //if (std::find(pairs.begin(), pairs.end(), p_) == pairs.end()) {
        if (pairs.count(p_) == 0) {
            return;
        }

        w += subs[p_].w+1;
    }

    if (j-i > 1) {
        assert(!subs[p].S.empty());

        if (!IsRefl(j, k, (subs[p].S.end()-1)->g)) {
            while (subs[p].S.size() > 1
                && !IsRefl(j, k, (subs[p].S.end()-2)->g)) {

                subs[p].S_tail.push_back(subs[p].S.back());
                subs[p].S.pop_back();
            }

            if (!subs[p].S.empty()
                && !IsRefl(i, (subs[p].S.end()-1)->f, k)) {
                a = (subs[p].S.end()-1)->f;
            } else {
                w++;
            }

        } else {
            w++;
        }
    }

    subs[{i, k}].AddPair({a, j}, w);
}

void Decomposer::Backw (int i, int j, int k) {
    //std::cout << "Back " << i << " " << j << " " << k << std::endl;

    Pair p(j, k);

    //if (std::find(pairs.begin(), pairs.end(), p) == pairs.end()) {
    if (pairs.count(p) == 0) {
        return;
    }

    int a = j;
    int w = subs[p].w;

    if (j-i > 1) {
        Pair p_(i, j);
        //if (std::find(pairs.begin(), pairs.end(), p_) == pairs.end()) {
        if (pairs.count(p_) == 0) {
            return;
        }

        w += subs[p_].w+1;
    }

    if (k-j > 1) {
        assert(!subs[p].S.empty());

        if (!IsRefl(j, (subs[p].S.begin())->f, i)) {
            while (subs[p].S.size() > 1
                && !IsRefl(j, (subs[p].S.begin()+1)->f, i)) {

                subs[p].S_head.push_back(subs[p].S.front());
                subs[p].S.pop_front();
            }

            if (!subs[p].S.empty()
                && !IsRefl(k, i, (subs[p].S.begin())->g)) {
                a = (subs[p].S.begin())->g;
            } else {
                w++;
            }

        } else {
            w++;
        }
    }

    subs[{i, k}].AddPair({j, a}, w);
}

void Decomposer::Recover (int i, int k) {

    if (k-i < 2) {
        return;
    }

    SubP &sA = subs[{i, k}];

    if (verts[i].refl) {
        int j = (sA.S.end()-1)->g;

        Recover(j, k);

        if (j-i > 1) {
            if ((sA.S.end()-1)->f != (sA.S.end()-1)->g) {
                SubP &sB = subs[{i, j}];
                sB.RestoreS();

                while (!sB.S.empty()
                    && (sA.S.end()-1)->f != (sB.S.end()-1)->f) {

                    sB.S.pop_back();
                }

            }

            Recover(i, j);
        }

    } else {
        int j = (sA.S.begin())->f;

        Recover(i, j);

        if (k-j > 1) {
            if ((sA.S.begin())->f != (sA.S.begin())->g) {
                SubP &sB = subs[{j, k}];
                sB.RestoreS();

                while (!sB.S.empty()
                    && (sA.S.begin())->g != (sB.S.begin())->g) {

                    sB.S.pop_front();
                }
            }

            Recover(j, k);
        }

    }

}

void Decomposer::Collect (int i, int k) {
    if (k-i < 2) {
        return;
    }

    SubP &s = subs[{i, k}];

    int j, a, b;

    if (verts[i].refl) {
        j = (s.S.end()-1)->g;
        a = j == (s.S.end()-1)->f;
        b = true;
    } else {
        j = (s.S.begin())->f;
        b = j == (s.S.begin())->g;
        a = true;
    }

    if (a && j-i > 1) {
        diags.push_back({i, j});
    }

    if (b && k-j > 1) {
        diags.push_back({j, k});
    }

    Collect(i, j);
    Collect(j, k);
}

void Decomposer::GetDecomposed (DecResType &res) {
    if (subs.empty()) {
        // keine refl vorhanden

        IdsType ids(poly.size());
        std::iota(ids.begin(), ids.end(), 0);

        res.push_back(ids);

        return;
    }

    // durchlauf

    for (int l = 3; l < num; l++) {
        for (int i = 0; i < num-l; i++) {
            if (verts[i].refl) {
                int k = i+l;

                Pair p(i, k);

                //if (std::find(pairs.begin(), pairs.end(), p) != pairs.end()) {
                if (pairs.count(p) == 1) {
                    if (verts[k].refl) {
                        for (int j = i+1; j < k; j++) {
                            Forw(i, j, k);
                        }
                    } else {
                        for (int j = i+1; j < k-1; j++) {
                            if (verts[j].refl) {
                                Forw(i, j, k);
                            }
                        }

                        Forw(i, k-1, k);
                    }

                }
            }
        }

        for (int k = l; k < num; k++) {
            if (verts[k].refl) {
                int i = k-l;

                Pair p(i, k);

                //if (std::find(pairs.begin(), pairs.end(), p) != pairs.end()) {
                if (pairs.count(p) == 1) {
                    if (!verts[i].refl) {
                        Backw(i, i+1, k);

                        for (int j = i+2; j < k; j++) {
                            if (verts[j].refl) {
                                Backw(i, j, k);
                            }
                        }
                    }
                }
            }
        }

    }

    // lösungen wiederherstellen
    Recover(0, num-1);

    // diagonalen sammeln
    Collect(0, num-1);

    std::sort(diags.begin(), diags.end(), [](const Pair &a, const Pair &b) {
        const int aG = -a.g,
            bG = -b.g;
        return std::tie(a.f, aG) < std::tie(b.f, bG);
    });

    for (auto& d : diags) {
        std::cout << d << std::endl;
    }

    // bildet polygone

    res.push_back({});

    int i = 0,
        p = 0,
        q = 0;

    std::vector<int> ps, rs;

    while (i < num) {
        IdsType &dec = res[p];

        if (dec.empty() || dec.back() != i) {
            dec.push_back(i);
        }

        if (!rs.empty() && i == diags[rs.back()].g) {
            if (dec.front() != diags[rs.back()].f) {
                dec.push_back(diags[rs.back()].f);
            }

            rs.pop_back();

            p = ps.back();
            ps.pop_back();

        } else if (q < diags.size() && i == diags[q].f) {
            if (dec.front() != diags[q].g) {
                dec.push_back(diags[q].g);
            }

            res.push_back({});

            rs.push_back(q);
            ps.push_back(p);

            p = ++q;
        } else {
            i++;
        }
    }

    for (auto& dec : res) {
        for (int& id : dec) {
            id = verts[id].id;
        }

        PolyType doly;
        for (int id : dec) {
            doly.push_back(poly[id]);
        }

        AddInternals(poly, doly);

        IdsType _dec;
        for (auto& p : doly) {
            _dec.push_back(p.id);
        }

        dec.swap(_dec);

    }

}

void SimpleRmInternals (VertsType6 &verts) {
    VertsType4 verts2(verts.begin(), verts.end());

    MarkInternals(verts2, NO_USE);
    RemoveInternals(verts2);

    VertsType6 verts3(verts2.begin(), verts2.end());
    verts.swap(verts3);

}

void Scale (PolyType &poly) {
    // skaliert das poly so, dass die kürzeste kante 1 lang ist

    int num = poly.size();

    std::vector<double> ls;

    for (int i = 0; i < num; i++) {
        int j = (i+1)%num;

        Point &pA = poly[i],
            &pB = poly[j];

        double v[] = {pB.x-pA.x, pB.y-pA.y},
            l = Normalize(v);

        ls.push_back(l);
    }

    double mn = *(std::min_element(ls.begin(), ls.end())),
        f = 1/mn;

    std::cout << "F " << f << std::endl;

    if (f > 1) {
        for (auto& p : poly) {
            p.pt[0] *= f;
            p.pt[1] *= f;
        }
    }

}
