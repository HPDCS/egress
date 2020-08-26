LPS = 1024

saved = [0] * LPS
deaths = [0] * LPS
with open("escape_stats.txt", "r") as f:
    for line in f:
        if line.startswith("-"):
            print("saved {}, deaths {}".format(sum(saved), sum(deaths)))
        else:
            lp_id, s, d = [int(tok) for tok in line.split()]
            saved[lp_id] = s
            deaths[lp_id] = d

print("saved {}, deaths {}".format(sum(saved), sum(deaths)))