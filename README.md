# Car Traffic Model

## Overview

This project is a C++ traffic simulation based on a PennDOT traffic model. The program simulates cars entering and leaving a road over a specified number of seconds and uses crash data from a SQLite database to calculate daily crash probabilities.

The main goal of this assignment was to significantly improve the program's running time without changing its behavior or output.

## Running the Program

Compile the program with:-
make

Run the simulation by providing the duration in seconds:-
./traffic-simulation 86400

For example, 86400 simulates one day and 604800 simulates one week.

To build and run the tests:-
make tests
./tests
