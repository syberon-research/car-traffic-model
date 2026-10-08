// author: Programmer Intern Jordan

#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <sqlite3.h>
#include <iostream>

#include "Sim_Math.h"


double power_recursive(double a, int b){
	if(b == 1){
		return a;
	}
	double tmp = a * power_recursive(a, b-1);
	return tmp;
}


double power(double a, int b){
	if(b == 0){
		return 1;
	}

	if(a == 0){
		return 0;
	}

	bool needs_inversion = false;

	if(b < 0){
		needs_inversion = true;
		b = -b;
	}
	
	double ans = power_recursive(a, b);

	if(needs_inversion){
		ans = 1.0 / ans;
	}

	return ans;
}


int poisson(int lambda){
	// Knuth's algorithm for Poisson random numbers.
	// Use the standard exponential function instead of
	// recursively calculating e^(-lambda).
	double L = std::exp(-lambda);

	int k = 0;
	double p = 1.0;

	while(p > L){
		k = k + 1;
		double u = (double)rand() / RAND_MAX;
		p *= u;
	}

	return k-1;
}


int binomial(int n, double p){
	// Binomial distribution, the number of successful trials in n Bernoulli events
	// Bernoulli events are simple:
		// each event is either a success or failure
		// all events are independent
		// all events have the same probability of success (p)
	// n gives the number of trials, in this case the number of cars
	// p gives the probability of success, in this case the probability an individual car exits

	int count = 0;

	for (int i = 0; i < n; i++){
		double u = (double)rand() / RAND_MAX;

		if(u < p){
			count = count + 1;
		}
	}
	
	return count;
}


int percentage(int portion, int overall){
	// returns portion / overall as a percentage
	// for example, if portion = 16 and overall = 82
	// then this function returns 19% = .019 = 16/82
	double per = (portion / static_cast<double>(overall)) * 100;
	return static_cast<int>(per);
}


bool crash_occurs(double prob_of_crash_today){
	if( ((double)rand() / RAND_MAX) < prob_of_crash_today ){
		return true;
	}
	return false;
}


int day_of_week_code(TimeCode cur_time){
	int cur_hour = cur_time.GetHours();
	cur_hour = cur_hour % 168;

	if(cur_hour <= 24){
		return 0; // Sunday
	} else if(cur_hour <= 48){
		return 1; // Monday
	} else if(cur_hour <= 72){
		return 2; // Tuesday
	} else if(cur_hour <= 96){
		return 3; // Wednesday
	} else if(cur_hour <= 120){
		return 4; // Thursday
	} else if(cur_hour <= 144){
		return 5; // Friday
	} else {
		return 6; // Saturday
	}
}


std::string day_of_week_name(int code){
	switch(code){
		case 0: return "Sunday";
		case 1: return "Monday";
		case 2: return "Tuesday";
		case 3: return "Wednesday";
		case 4: return "Thursday";
		case 5: return "Friday";
		case 6: return "Saturday";
		default: 
			throw std::invalid_argument(
				"Invalid day of week code: " + std::to_string(code)
			);
	}
}


int prepare(sqlite3* db, const char* query, sqlite3_stmt** stmt){
	int rc = sqlite3_prepare_v2(db, query, -1, stmt, 0);

	if(rc != SQLITE_OK){
		sqlite3_close(db);
		throw std::runtime_error("Could not prepare database query.");
	}

	return rc;
}


std::unordered_map<std::string, double> build_crash_prob_map(){

	sqlite3* db;

	int rc = sqlite3_open("CRASH_LANCASTER_2024.db", &db);

	if(rc){
		throw std::invalid_argument(
			"Could not open CRASH_LANCASTER_2024.db database file!"
		);
	}


	/*
	 * Get the total number of crashes.
	 *
	 * This is still one simple database query.
	 */
	const char* total_sql =
		"SELECT COUNT(*) FROM incidents;";

	double total_crashes = 0.0;

	sqlite3_stmt* stmt = nullptr;

	rc = prepare(db, total_sql, &stmt);

	if(rc == SQLITE_OK){

		rc = sqlite3_step(stmt);

		if(rc == SQLITE_ROW){

			total_crashes =
				static_cast<double>(
					sqlite3_column_int(stmt, 0)
				);
		}
	}

	sqlite3_finalize(stmt);


	/*
	 * The original code did a database query for every CRN
	 * for every day of the week.
	 *
	 * Instead, this single query reproduces the same counting:
	 *
	 * 1. Find CRNs belonging to incidents with automobiles.
	 * 2. Match those CRNs back to the incidents table.
	 * 3. Count the matching incidents grouped by day of week.
	 *
	 * The join preserves duplicate CRNs in the same way as
	 * the original nested loops.
	 */
	const char* grouped_sql =
		"SELECT i.DAY_OF_WEEK, COUNT(*) "
		"FROM incidents AS i "
		"INNER JOIN ("
		"    SELECT CRN "
		"    FROM incidents "
		"    WHERE AUTOMOBILE_COUNT != 0"
		") AS c "
		"ON i.CRN = c.CRN "
		"GROUP BY i.DAY_OF_WEEK;";


	std::vector<int> day_counts(7, 0);

	rc = prepare(db, grouped_sql, &stmt);

	while((rc = sqlite3_step(stmt)) == SQLITE_ROW){

		int day =
			sqlite3_column_int(stmt, 0);

		int count =
			sqlite3_column_int(stmt, 1);

		if(day >= 1 && day <= 7){

			day_counts[day - 1] = count;
		}
	}


	if(rc != SQLITE_DONE){

		std::cout
			<< "Error: "
			<< sqlite3_errmsg(db)
			<< std::endl;

		sqlite3_finalize(stmt);
		sqlite3_close(db);

		throw std::runtime_error(
			"Error iterating over DB query results."
		);
	}


	sqlite3_finalize(stmt);
	sqlite3_close(db);


	/*
	 * Build the same probability map as before.
	 */
	std::unordered_map<std::string, double> crash_prob_map;

	for(int dow = 0; dow < 7; dow++){

		crash_prob_map[day_of_week_name(dow)] =
			static_cast<double>(day_counts[dow])
			/ total_crashes;
	}


	return crash_prob_map;
}