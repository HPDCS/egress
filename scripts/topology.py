toggle_l = [
256, 276, 279, 288,
512, 521, 524, 532, 535, 544,
564, 565,
595, 596,
628, 629,
659, 660,
692, 693,
723, 724,
756, 757,
768, 785, 788, 797, 
820, 821,
851, 852,
884, 885,
915, 916,
948, 949,
979, 980,
1012, 1013, 2000]


with open("topology.json", "w") as f:
    f.write("{\"values\":[")
    o = False
    j = 0   
    for i in range(1023):
        if toggle_l[j] == i:
            o = not o
            j += 1
        if o:
            f.write("1.0,")
        else:
            f.write("0.0,")
        
    
    if o:
        f.write("1.0]}")
    else:
        f.write("0.0], \"regions_count\": 1024, \"type\": \"obstacles\", \"geometry\": \"hexagons\"}")
