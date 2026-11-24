import numpy as np

s_values = [1000]  # Scale
n_values = [100,200] # Items
a_values = [a / 10 for a in range(5, 50+1, 1)]
c_values = [c / 10 for c in range(10, 20+1, 2)]

rng = np.random.default_rng(seed=0)

# Instrance generations
for s in s_values:
    for n in n_values: 
        for a in a_values:       
            for c in c_values:
                items = []
                while len(items) < n:
                    sample = rng.weibull(a) 
                    item = int(s * sample)
                    if item > 0:
                        items.append(item)
                items.sort(reverse=True)
                capacity = int(round(c * max(items)));
                filename = "weibull_n{:03}_a{:02}_c{:02}.BPP".format(n,int(a*10),int(c*10))
                with open(filename, "w") as f:
                    f.write("{}\n".format(n))
                    f.write("{}\n".format(capacity))
                    for i in items:
                        f.write("{}\n".format(i))
