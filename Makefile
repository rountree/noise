all:
	gcc -static -std=c23 -Wall -Wextra -Werror -O3 -o noise noise.c -lpthread

poodle:
	#srun -N 1-1 --exclusive -t 10:00 taskset --cpu-list 0 ./noise > poodle.0.R
	#srun -N 1-1 --exclusive -t 10:00 taskset --cpu-list 0-1 ./noise > poodle.0-1.R
	#srun -N 1-1 --exclusive -t 10:00 taskset --cpu-list 0-3 ./noise > poodle.0-3.R
	#srun -N 1-1 --exclusive -t 10:00 taskset --cpu-list 0-7 ./noise > poodle.0-7.R
	#srun -N 1-1 --exclusive -t 10:00 taskset --cpu-list 0-15 ./noise > poodle.0-15.R
	#srun -N 1-1 --exclusive -t 10:00 taskset --cpu-list 0-31 ./noise > poodle.0-31.R
	#srun -N 1-1 --exclusive -t 10:00 taskset --cpu-list 0-63 ./noise > poodle.0-63.R
	#srun -N 1-1 --exclusive -t 10:00 taskset --cpu-list 0-63 ./noise > poodle.0-63.R
	#srun -N 1-1 --exclusive -t 10:00 taskset --cpu-list 0-127 ./noise > poodle.0-127.R
	srun -N 1-1 --exclusive -t 10:00 ./noise > poodle.all.R

ruby:
	srun -ppdebug -A asccasc -N1-1 -t 10:00 ./noise > ruby.R

tioga:
	srun -ppci -N1  -A asccasc -t 10:00  ./noise > tioga.R
