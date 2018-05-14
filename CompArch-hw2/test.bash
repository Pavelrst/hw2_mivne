#!bin/bash

make

echo TEST_RUN_1 - basic test
./bp_main examples/example1_in.txt > ./my_out1.txt
./bp_main examples/example2_in.txt > ./my_out2.txt
./bp_main examples/example3_in.txt > ./my_out3.txt
./bp_main examples/example4_in.txt > ./my_out4.txt
./bp_main examples/example5_in.txt > ./my_out5.txt
./bp_main examples/example6_in.txt > ./my_out6.txt

echo TEST_RUN_2 - facebook test
./bp_main rand_tests/rand_tests/ex1_in.txt > ./my_rand_out1.txt
./bp_main rand_tests/rand_tests/ex2_in.txt > ./my_rand_out2.txt
./bp_main rand_tests/rand_tests/ex3_in.txt > ./my_rand_out3.txt
./bp_main rand_tests/rand_tests/ex4_in.txt > ./my_rand_out4.txt
./bp_main rand_tests/rand_tests/ex5_in.txt > ./my_rand_out5.txt
./bp_main rand_tests/rand_tests/ex6_in.txt > ./my_rand_out6.txt
./bp_main rand_tests/rand_tests/ex7_in.txt > ./my_rand_out7.txt
./bp_main rand_tests/rand_tests/ex8_in.txt > ./my_rand_out8.txt
./bp_main rand_tests/rand_tests/ex9_in.txt > ./my_rand_out9.txt
./bp_main rand_tests/rand_tests/ex10_in.txt > ./my_rand_out10.txt
./bp_main rand_tests/rand_tests/ex11_in.txt > ./my_rand_out11.txt
./bp_main rand_tests/rand_tests/ex12_in.txt > ./my_rand_out12.txt
./bp_main rand_tests/rand_tests/ex13_in.txt > ./my_rand_out13.txt
./bp_main rand_tests/rand_tests/ex14_in.txt > ./my_rand_out14.txt
./bp_main rand_tests/rand_tests/ex15_in.txt > ./my_rand_out15.txt
./bp_main rand_tests/rand_tests/ex16_in.txt > ./my_rand_out16.txt
./bp_main rand_tests/rand_tests/ex17_in.txt > ./my_rand_out17.txt
./bp_main rand_tests/rand_tests/ex18_in.txt > ./my_rand_out18.txt
./bp_main rand_tests/rand_tests/ex19_in.txt > ./my_rand_out19.txt
./bp_main rand_tests/rand_tests/ex20_in.txt > ./my_rand_out20.txt

echo TEST_DIFF_1 - basic test
diff ./my_out1.txt ./examples/example1_out.txt
diff ./my_out2.txt ./examples/example2_out.txt
diff ./my_out3.txt ./examples/example3_out.txt
diff ./my_out4.txt ./examples/example4_out.txt
diff ./my_out5.txt ./examples/example5_out.txt
diff ./my_out6.txt ./examples/example6_out.txt

echo TEST_DIFF_2 - facebook test
diff ./my_rand_out1.txt ./rand_tests/rand_results/ex1_out.txt
diff ./my_rand_out2.txt ./rand_tests/rand_results/ex2_out.txt
diff ./my_rand_out3.txt ./rand_tests/rand_results/ex3_out.txt
diff ./my_rand_out4.txt ./rand_tests/rand_results/ex4_out.txt
diff ./my_rand_out5.txt ./rand_tests/rand_results/ex5_out.txt
diff ./my_rand_out6.txt ./rand_tests/rand_results/ex6_out.txt
diff ./my_rand_out7.txt ./rand_tests/rand_results/ex7_out.txt
diff ./my_rand_out8.txt ./rand_tests/rand_results/ex8_out.txt
diff ./my_rand_out9.txt ./rand_tests/rand_results/ex9_out.txt
diff ./my_rand_out10.txt ./rand_tests/rand_results/ex10_out.txt
diff ./my_rand_out11.txt ./rand_tests/rand_results/ex11_out.txt
diff ./my_rand_out12.txt ./rand_tests/rand_results/ex12_out.txt
diff ./my_rand_out13.txt ./rand_tests/rand_results/ex13_out.txt
diff ./my_rand_out14.txt ./rand_tests/rand_results/ex14_out.txt
diff ./my_rand_out15.txt ./rand_tests/rand_results/ex15_out.txt
diff ./my_rand_out16.txt ./rand_tests/rand_results/ex16_out.txt
diff ./my_rand_out17.txt ./rand_tests/rand_results/ex17_out.txt
diff ./my_rand_out18.txt ./rand_tests/rand_results/ex18_out.txt
diff ./my_rand_out19.txt ./rand_tests/rand_results/ex19_out.txt
diff ./my_rand_out20.txt ./rand_tests/rand_results/ex20_out.txt