/*
This c program is testing the Predictor class used to predict 
the motor controllers current position between position updates
*/

#include "../Predictor.h"
#include "../DriverConstants.h"
#include <csv_parser/csv_parser.hpp>

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <climits>
#include <list>

#include "SubSetTest.h"


using namespace std;

const u1 speedMultiply = 1;
const bool WriteToFile = false;
const bool GetMaxes = true;

const u1 tickCol = 0;
const u1 phaseCol = 1;

ofstream output;

csv_parser file_parser;

void setUp(){

	/* Declare the variables to be used */
	const char * filename = "magData.tsv";
	const char field_terminator = '\t';
	const char line_terminator	= '\n';
	const char enclosure_char	 = '"';


	SubSetTest::init();

	/* Define how many records we're gonna skip. This could be used to skip the column definitions. */
	file_parser.set_skip_lines(1);

	/* Specify the file to parse */
	file_parser.init(filename);

	/* Here we tell the parser how to parse the file */
	file_parser.set_enclosed_char(enclosure_char, ENCLOSURE_OPTIONAL);

	file_parser.set_field_term_char(field_terminator);

	file_parser.set_line_term_char(line_terminator);


	if(WriteToFile){
		output.open("out.csv");
		output <<"tick,predicted,actual (approx),error,error^2"<<endl;
	}

}

double max = 0;

double differnecewithwrap(double a, double b){
	if (abs(int(a - b)) > (DriverConstants::StepsPerCycle / 2)){
		if ((b - a) > 0)
			return b-a-DriverConstants::StepsPerCycle;
		else
			return b-a+DriverConstants::StepsPerCycle;

	}

	return b-a;
}

s2 maxDelta = 0;
ull maxDeltaPos = 0;

s2 maxDeltaDelta = 0;
ull maxDeltaDeltaPos = 0;

int main(){

	setUp();
	double errorSum = 0;
	ull num = 0;

	csv_row previousRow = file_parser.get_row();
	csv_row row = file_parser.get_row();

	Predictor::init(stoi(previousRow[phaseCol]));

	new SubSetTest("stopped", 0, 8000);
	new SubSetTest("nominal", 10000, 10100);
	new SubSetTest("HighSpeed", 16414, 16514);

	s2 prevDelta = 0;

	while(file_parser.has_more_rows()){

		// cout << stoi(row[phaseCol]) << endl;

		for (int i = 0; i < DriverConstants::PredictsPerValue; i++){
			u2 pred = Predictor::predict();

			s2 delta = differnecewithwrap(stoi(row[phaseCol]), stoi(previousRow[phaseCol]));

			if (GetMaxes){
				if(abs(delta) > abs(maxDelta)){
					maxDelta = delta;
					maxDeltaPos = stoi(previousRow[tickCol]);
				}
			
				//ignore values where interpreter gets values wrong
				if (stoi(previousRow[tickCol]) <= 16400 || stoi(previousRow[tickCol]) >= 16500){
					if ((delta > 0 && prevDelta < 0) || (delta < 0 && prevDelta > 0)){
						if(abs(delta-prevDelta) > abs(maxDeltaDelta)){
							maxDeltaDelta = delta-prevDelta;
							maxDeltaDeltaPos = stoi(previousRow[tickCol]);
						}
					}
				}
				prevDelta = delta;
			}
			
			double partialStep = ((double)i / DriverConstants::PredictsPerValue);

			double actual = stoi(previousRow[phaseCol]) - delta * partialStep;

			if (actual < 0)
				actual += DriverConstants::StepsPerCycle;
			else if (actual >= DriverConstants::StepsPerCycle)
				actual -= DriverConstants::StepsPerCycle;

			double error = differnecewithwrap(pred, actual);
			double e2 = error * error;

			errorSum += e2;
			num++;

			SubSetTest::runTest(stoi(previousRow[tickCol])+partialStep, e2);

			if(WriteToFile){
				output << stoi(previousRow[tickCol]) / speedMultiply
					//adds the decimal portion as a string or the full number is not reccorded in large numbers
					<< to_string(partialStep).substr(1,4) << ','
					<<pred<<','
					<<actual<<','
					<<error<<','
					<<e2
					<<endl;
			}
		}
	
		// cout<<endl;
		
		/* Get the record */
	previousRow = row;

	//dumb way of atifically speeding up motor
	//this way does not keep the refrence data in between computed cycles
	//in other words not as acurate to check again
	for (int a = 0; a < speedMultiply; a++)
		row = file_parser.get_row();

	//double check for skipped ticks
	if (stoi(row[tickCol]) - stoi(previousRow[tickCol]) != 1){
		cout<<"skipped tick(s) ";
		for (int i = stoi(previousRow[tickCol])+1; i < stoi(row[tickCol]); i++){
			cout << i<< ' ';
		}
		cout << endl;
	}

	Predictor::freshPhase(stoi(previousRow[phaseCol]));

		// cout << endl<< stoi(lastrow[phaseCol]) << '\t';
		// output << previousRow[tickCol]<< ","<<stoi(previousRow[phaseCol])<<"," << stoi(previousRow[phaseCol])<<endl;

	}
	cout << errorSum / num << endl;

	if (GetMaxes){
		cout<<"maxDelta @ " << maxDeltaPos<<endl;
		cout<<"maxDeltaDelta @ " << maxDeltaDeltaPos<<endl;
	}

	// cout << s->report();
	cout << SubSetTest::reportTests() << endl;

}

