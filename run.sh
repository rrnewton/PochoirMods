#!/bin/bash
step_low=200
step_high=4000
step_step=200
size_low=200
size_high=4000
step_size=200

for ((step=${step_low}; step <= ${step_high}; step += ${step_step})) do
	for ((size=${size_low}; size <= ${size_high}; size += ${step_size})) do
		echo "./loop $size $step"
		./loop $size $step
		echo "./obase $size $step"
		./obase $size $step
	done
done
