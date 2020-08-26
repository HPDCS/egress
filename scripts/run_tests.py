import subprocess

RUNS_PER_POINT = 5
RANGE_GRANULARITY = 15
ARGUMENTS_TO_TEST = [ ("env-p", "knowledge", 0, 1.0)]
LPS = 1024
BASE_CMD = ["./a.out", "--serial", "--lp", str(LPS)]


def extract_stats():
    saved = 0
    deaths = 0
    with open("escape_stats.txt", "r") as f:
        for line in f:
            s, d = [int(tok) for tok in line.split()]
            saved = s
            deaths = d

    with open("time.txt", "r") as f:
        simt = float(f.read())

    return saved, deaths, simt


for tetra in ARGUMENTS_TO_TEST:
    arg, f_name, start, end = tetra
    datas = []
    for i in range(RANGE_GRANULARITY + 1):
        saved_stat = 0
        deaths_stat = 0
        times_stat = 0.0
        arg_val = start + ((end - start) * i / RANGE_GRANULARITY)

        command = BASE_CMD + ["--" + arg, str(arg_val)]
        for _ in range(RUNS_PER_POINT):
            process = subprocess.run(command)
            this_saved, this_deaths, this_simt = extract_stats()
            saved_stat += this_saved
            deaths_stat += this_deaths
            times_stat += this_simt
        saved_stat /= RUNS_PER_POINT
        deaths_stat /= RUNS_PER_POINT
        times_stat /= RUNS_PER_POINT
        datas.append((arg_val, saved_stat, deaths_stat, times_stat))

    with open(f_name + "-f.dat", "w") as f:
        for triple in datas:
            arg_val, saved_stat, deaths_stat, times_stat = triple
            fatalities_p = deaths_stat / (saved_stat + deaths_stat)
            f.write("{}\t{}\n".format(arg_val, fatalities_p))

    with open(f_name + "-s.dat", "w") as f:
        for triple in datas:
            arg_val, saved_stat, deaths_stat, times_stat = triple
            st_stat = saved_stat / times_stat
            f.write("{}\t{}\n".format(arg_val, st_stat))

    with open(f_name + "-t.dat", "w") as f:
        for triple in datas:
            arg_val, saved_stat, deaths_stat, times_stat = triple
            f.write("{}\t{}\n".format(arg_val, times_stat))
    
