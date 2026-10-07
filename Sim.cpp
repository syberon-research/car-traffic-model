// author: Programmer Intern Jordan

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>
#include <cmath>
#include <algorithm>

#include "Sim_Math.h"
#include "TimeCode.h"
#include "TimeCode_Tests.h"
#include "BigInteger.h"

const int CAP = 200;
const int DURATION_10MIN = 600;
const int DURATION_1DAY = 86400;
const int DURATION_1WK = 604800;


// Store only the number of cars.
// The simulation second is simply the vector index.
struct results{
    TimeCode duration;
    int road_capacity;
    int new_car_rate;
    int num_crashes;
    std::string total_cars;

    // Number of seconds spent with each number of cars.
    std::unordered_map<int, size_t> count_times_map;

    std::vector<int> x_data;
    std::vector<int> y_data;

    TimeCode plotted_duration;
};


void generateHTMLoutput(
    const results& data,
    std::string filename = "plot.html"
){
    std::ofstream file(filename);

    file << "<!DOCTYPE html>\n";
    file << "<html>\n";
    file << "<head>\n";
    file << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n";
    file << "</head>\n";
    file << "<body>\n";

    file << "<canvas id=\"myChart\"></canvas>\n";

    file << "<script>\n";

    file << "const xValues = [";

    for(size_t i = 0; i < data.x_data.size(); i++){
        file << data.x_data[i];

        if(i + 1 < data.x_data.size()){
            file << ",";
        }
    }

    file << "];\n";

    file << "const yValues = [";

    for(size_t i = 0; i < data.y_data.size(); i++){
        file << data.y_data[i];

        if(i + 1 < data.y_data.size()){
            file << ",";
        }
    }

    file << "];\n";

    file << "\n";

    file << "new Chart(\"myChart\", {\n";
    file << "  type: \"line\",\n";
    file << "  data: {\n";
    file << "    labels: xValues,\n";
    file << "    datasets: [{\n";
    file << "      fill: false,\n";
    file << "      lineTension: 0,\n";
    file << "      backgroundColor: \"rgba(0,0,255,1.0)\",\n";
    file << "      borderColor: \"rgba(0,0,255,0.1)\",\n";
    file << "      data: yValues\n";
    file << "    }]\n";
    file << "  },\n";
    file << "  options: {\n";
    file << "    legend: {display: false},\n";
    file << "    scales: {\n";
    file << "      yAxes: [{ticks: {min: 0, max: 200}}],\n";
    file << "      xAxes: [{scaleLabel: {display: true, labelString: 'Time (seconds)'}}]\n";
    file << "    }\n";
    file << "  }\n";
    file << "});\n";

    file << "</script>\n";
    file << "</body>\n";
    file << "</html>\n";

    file.close();

    std::cout
        << " HTML file generated successfully: "
        << filename
        << std::endl;
}


void generateTerminalOutput(const results& data){

    std::cout
        << " duration: "
        << data.duration.GetHours()
        << "hrs "
        << data.duration.GetMinutes()
        << "mins "
        << data.duration.GetSeconds()
        << "secs"
        << std::endl;

    std::cout
        << " road capacity: "
        << data.road_capacity
        << std::endl;

    std::cout
        << " avg new cars / second: "
        << data.new_car_rate
        << std::endl;

    std::cout
        << " number of crashes: "
        << data.num_crashes
        << std::endl;

    std::cout
        << " total number of cars simulated: "
        << data.total_cars
        << std::endl;


    auto get_count = [&](int car_count) -> size_t {

        auto it = data.count_times_map.find(car_count);

        if(it == data.count_times_map.end()){
            return 0;
        }

        return it->second;
    };


    std::cout
        << " The number of moments (individual seconds) in which there were 0 cars: "
        << get_count(0)
        << std::endl;

    std::cout
        << " The number of moments (individual seconds) in which there was exactly 1 car: "
        << get_count(1)
        << std::endl;

    std::cout
        << " The number of moments (individual seconds) in which there were only 2 cars: "
        << get_count(2)
        << std::endl;


    size_t ten_or_less = 0;

    for(int i = 0; i <= 10; i++){
        ten_or_less += get_count(i);
    }


    double percent_ten_or_less =
        (ten_or_less /
         static_cast<double>(data.duration.GetTimeCodeAsSeconds()))
        * 100.0;


    std::cout
        << " The number of moments (individual seconds) in which there were 10 cars or fewer: "
        << ten_or_less
        << " (about "
        << static_cast<int>(percent_ten_or_less)
        << "% of the entire simulation time.)"
        << std::endl;
}


int main(int argc, char* argv[]){

    if(argc != 2){
        std::cout
            << "Usage: ./traffic-simulation <duration in seconds>"
            << std::endl;

        return 1;
    }

    int DURATION = std::stoi(argv[1]);

    TimeCode dur = TimeCode(0, 0, DURATION);

    std::cout
        << "--- Traffic Simulator ---"
        << std::endl;

    std::cout
        << "Computing crash probabilities from database..."
        << std::endl;

    std::unordered_map<std::string, double> crash_prob_map =
        build_crash_prob_map();

    for(auto& pair : crash_prob_map){
        std::cout
            << "Day: "
            << pair.first
            << ", probability of a crash: "
            << pair.second
            << std::endl;
    }

    std::cout
        << "Simulating traffic..."
        << std::endl;


    unsigned long long total_num_cars = 0;

    /*
     * We only need the number of cars for each second.
     *
     * The old implementation stored:
     *     TimeCode + int
     *
     * for every second, which caused millions of TimeCode
     * copies. The second itself is already represented by
     * the vector index.
     */
    std::vector<int> data;

    if(DURATION <= DURATION_1WK){
        data.reserve(DURATION);
    }


    int num_cars = 0;
    int crash_count = 0;
    int crash_timer = 0;


    double prob_of_crash_today =
        crash_prob_map.at(
            day_of_week_name(0)
        );


    int previous_progress = -1;


    for(int current_seconds = 0;
        current_seconds < DURATION;
        current_seconds++){

        int progress =
            percentage(
                current_seconds,
                DURATION
            );

        if(progress != previous_progress){

            std::cout
                << "\r"
                << progress
                << "%"
                << std::flush;

            previous_progress = progress;
        }


        int num_new_cars =
            poisson(4);


        if(num_cars >= CAP){
            num_new_cars = 0;
        }


        total_num_cars += num_new_cars;


        double cur_p_car_exits =
            0.298 *
            (1.0 - double(num_cars) / CAP);


        int num_cars_exiting =
            binomial(
                num_cars,
                cur_p_car_exits
            );


        if(crash_timer > 0){
            num_cars_exiting = 0;
        }

        crash_timer--;


        /*
         * A new day begins every 86400 seconds.
         *
         * The original simulation starts on Sunday,
         * so calculate the day directly instead of
         * constructing a TimeCode.
         */
        if(current_seconds % DURATION_1DAY == 0){

            int day_code =
                (current_seconds / DURATION_1DAY) % 7;

            prob_of_crash_today =
                crash_prob_map.at(
                    day_of_week_name(day_code)
                );


            bool crash =
                crash_occurs(
                    prob_of_crash_today
                );


            if(crash){
                crash_count++;
                crash_timer += DURATION_10MIN;
            }
        }


        int cand_num_cars =
            num_cars
            + num_new_cars
            - num_cars_exiting;


        if(cand_num_cars < 0){
            num_cars = 0;
        }
        else{
            num_cars = cand_num_cars;
        }


        data.push_back(num_cars);
    }


    std::cout << std::endl;


    std::cout
        << "Computing sample statistics..."
        << std::endl;


    /*
     * We only need counts, not a vector of TimeCodes.
     *
     * The previous version stored every TimeCode for every
     * car-count category. That created a large number of
     * unnecessary object constructions and copies.
     */
    std::unordered_map<int, size_t> count_times;


    previous_progress = -1;


    for(size_t i = 0; i < data.size(); i++){

        int progress =
            percentage(
                static_cast<int>(i),
                static_cast<int>(data.size())
            );


        if(progress != previous_progress){

            std::cout
                << "\r"
                << progress
                << "%"
                << std::flush;

            previous_progress = progress;
        }


        count_times[data[i]]++;
    }


    std::cout << std::endl;


    std::cout
        << "---Simulation Finished---"
        << std::endl;


    results res;

    res.duration = dur;
    res.road_capacity = CAP;
    res.new_car_rate = 4;
    res.num_crashes = crash_count;

    res.total_cars =
        std::to_string(total_num_cars);

    res.count_times_map =
        std::move(count_times);


    /*
     * Plot 10 minutes of traffic.
     *
     * The original program always started at second 7200.
     * For a normal 604800-second simulation this remains
     * exactly the same.
     *
     * For shorter test runs, start at 0 so that we do not
     * access an element that doesn't exist.
     */

    size_t start = 7200;

    if(data.size() <= start){
        start = 0;
    }


    size_t end =
        std::min(
            start + static_cast<size_t>(DURATION_10MIN),
            data.size()
        );


    std::vector<int> x_data;
    std::vector<int> y_data;

    x_data.reserve(end - start);
    y_data.reserve(end - start);


    for(size_t i = start; i < end; i++){

        /*
         * The vector index is exactly the simulated second.
         * No TimeCode construction is necessary.
         */
        x_data.push_back(
            static_cast<int>(i)
        );

        y_data.push_back(
            data[i]
        );
    }


    res.x_data = std::move(x_data);
    res.y_data = std::move(y_data);


    res.plotted_duration =
        TimeCode(0, 0, DURATION_10MIN);


    res.plotted_duration
        .WasteTimeAndBeSlow();


    generateTerminalOutput(res);

    generateHTMLoutput(res);


    return 0;
}