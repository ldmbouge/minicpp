# Maximum independent set instances

**This is not the complete benchmark set.** Only the 20 smallest instances are
kept in the repository, to keep clones and build folders small. The other 44
are listed at the bottom of this file.

These graphs are the complements of the DIMACS maximum clique benchmarks
(Second DIMACS Implementation Challenge). An independent set in one of these
files is therefore a clique in the original DIMACS graph, and the maximum
independent set size equals the clique number of the original instance. For
example, `MANN_a9` has 45 nodes and 72 edges here, against 990 - 72 = 918 edges
in the original; its maximum independent set has size 16.

Format: DIMACS edge format. Lines starting with `c` are comments, the
`p edge <nodes> <edges>` line gives the size, and each `e <u> <v>` line is an
edge between 1-based nodes `u` and `v`.

Used by `examples/mdd/MISPfile.cpp`, whose default instance is
`data/misp/brock200_1.clq` (run it from the build folder, where CMake copies
`examples/data`). For example:

    ./MISPfile -w32 -m1 -r200 -i1000 -fdata/misp/MANN_a9.clq

## Included (smallest first)

| Instance | Nodes | Edges |
|---|---:|---:|
| MANN_a9 | 45 | 72 |
| johnson8-2-4 | 28 | 168 |
| hamming6-2 | 64 | 192 |
| johnson8-4-4 | 70 | 560 |
| MANN_a27 | 378 | 702 |
| hamming8-2 | 256 | 1024 |
| hamming6-4 | 64 | 1312 |
| johnson16-2-4 | 120 | 1680 |
| san200_0.9_2 | 200 | 1990 |
| san200_0.9_3 | 200 | 1990 |
| san200_0.9_1 | 200 | 1990 |
| sanr200_0.9 | 200 | 2037 |
| MANN_a45 | 1035 | 1980 |
| keller4 | 171 | 5100 |
| brock200_1 | 200 | 5066 |
| hamming10-2 | 1024 | 5120 |
| san200_0.7_1 | 200 | 5970 |
| san200_0.7_2 | 200 | 5970 |
| sanr200_0.7 | 200 | 6032 |
| brock200_4 | 200 | 6811 |

## Not included

brock200_2, brock200_3, brock400_1, brock400_2, brock400_3, brock400_4,
brock800_1, brock800_2, brock800_3, brock800_4, c-fat200-1, c-fat200-2,
c-fat200-5, c-fat500-1, c-fat500-2, c-fat500-5, c-fat500-10, hamming8-4,
hamming10-4, johnson32-2-4, keller5, p_hat300-1, p_hat300-2, p_hat300-3,
p_hat500-1, p_hat500-2, p_hat500-3, p_hat700-1, p_hat700-2, p_hat700-3,
p_hat1000-1, p_hat1000-2, p_hat1000-3, p_hat1500-1, p_hat1500-2, p_hat1500-3,
san400_0.5_1, san400_0.7_1, san400_0.7_2, san400_0.7_3, san400_0.9_1, san1000,
sanr400_0.5, sanr400_0.7.
