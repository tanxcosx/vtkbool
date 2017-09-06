#!/usr/bin/env python
# *-* coding: UTF-8 *-*

import json

from vis_poly import vis_poly_wrapper
from tools import to_abs_path

with open('complex.json', 'r') as f:
    polys = json.load(f)['polys']

    for i, poly in enumerate(polys):
        num = len(poly)

        for j in range(1, num):
            poly[j][0] += poly[j-1][0]
            poly[j][1] += poly[j-1][1]

        all_res = {}

        for j in range(num):
            all_res[j] = to_abs_path([ p['pt'] for p in vis_poly_wrapper(poly, j) ])

        with open('data_files/data_%i.js' % i, 'w') as out:
            out.write('var polys = %s;' % json.dumps(all_res))
