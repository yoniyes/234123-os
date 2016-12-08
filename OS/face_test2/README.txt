Usage:make and ./Tester

Info about parameters in settings-hw2.txt:
1. Iterations - the ShortGeneralTest creates a short process, run it until it becomes overdue and make sure 
it becomes overdue and becomes short, then run it untill it is OTHER and checks that it is possbile to declare it 
SHORT again. It repeat for "Iterations" amount of times. Don't put it too high - it will take much time to finish.

2. nices - list of nices to test arbitration in Arbitration test and ShortVsFifo. put appropriate nice values separated by comma. The amount of 
nices is the amount of processes that will be created for each type(Short,FIFO,Other) in these tests.

Note: the "running" of the process is consumed by some stupid computation(funcToCompute). I made the correct number of iterations 
according to my PC but it might make the ShortGeneralTest fail if your PC is much faster/slower. You can try to adjust it yourself or just skip this test.