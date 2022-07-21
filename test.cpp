#include "llcirq.h"
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <thread>
#include <future>

#define TESTQ_1 8
#define TESTQ_2 1000

using namespace std;

class QTester {
public:
	llcir_qp q;
	vector<char> datum_1; 
	vector<char> datum_2; 
	char check;

	QTester(llcir_qp qp) : q(qp) {
		datum_1 = vector<char>({'a','b','c','d','e','f','g','h','i'});
		datum_2 = vector<char>({'x','y','z'});
	} 

	bool test_basic_full() {
		bool rtv = true;
		for(int i = 0; i < sizeof(datum_1); i++){
			if(enqueue(q,&datum_1[i]) == false && i < q->num_elements - 1) {
				std::cout << "basic_full: Attempt to enqueue element " << i << "failure." << std::endl;
				rtv = false;
			}
		}
	
		for(int i = 0; i < sizeof(datum_1); i++){
			if(!dequeue(q,&check) && i < TESTQ_1 - 1) {
				std::cout << "basic_full: Attempt to check element " << i << "failure." << std::endl;
				rtv = false;
			}
		}
		return rtv;
	}

	bool test_basic_extra(){
		bool rtv = true;
		for(int i = 0; i < sizeof(datum_2); i++){
			enqueue(q,&datum_2[i]);
		}
		for(int i = 0; i < 10; i++){
			if(!dequeue(q,&check) && i < datum_2.size()){
				rtv = false;
				std::cout << "basic_extra: Attempt to check element " << i << "failure." << std::endl;
			}
			if(i < sizeof(datum_2) && check != datum_2[i]){
				rtv = false;
				std::cout << "Data check failed, expected to return " << datum_2[i] << " but returned " << check << std::endl;
			}
		}
		return rtv;
	}

	void test_char_enqueuer(const vector<char> &data){
		for(size_t i = 0; i < data.size(); i++){
			while(!enqueue(q,(void *)&data[i])) {
				cout << "Enqueuer " << this_thread::get_id() << " is stalling on " << data[i] << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));	
			}
			if((i % 100) == 0) {
				cout << "Enqueuer " << this_thread::get_id() << " is enqueueing" << endl;
			}
		}
	}
	
	bool test_char_dequeuer(const vector<char> &d){
		bool rtv = true;
		vector<char> data(d);
		while(data.size()){
			char c;
			while(!dequeue(q,&c)) std::this_thread::sleep_for(std::chrono::milliseconds(10));	
			//cout << "Dequeued char " << c << endl;
			bool found = false;
			for(int i = 0; i < data.size(); i ++){
				if(data[i] == c) {
					//cout << "Erasing " << data[i] << endl;
					data.erase(data.begin() + i);
					found = true;
					break;
				}
			}
			if(!found) {
				cout << "Unable to find char " << c << " in expected dataset, failure!" << endl;
				rtv = false;
				break;
			}
		}
		return rtv;
	}

};

bool basic_tests() {
	bool rtv = true;
	llcir_qp q = init_llcirq(1,TESTQ_1,1);
	QTester tester(q);

	if(!tester.test_basic_full()) {
		cout << "Test basic full failure!" << endl;
		rtv = false;
	}
	if(!tester.test_basic_extra()) {
		cout << "Test basic extra failure!" << endl;
		rtv = false;
	}
	return rtv;
}

bool thread_tests() {
	bool rtv = true;
	llcir_qp q = init_llcirq(1,TESTQ_2,1);
	QTester tester(q);

	vector<char> set1;
	vector<char> set2;
	vector<char> set3;

	for(size_t i = 0; i < 10000; i++){
		set1.push_back(i % 26 + 'a');
		set2.push_back((i + 5) % 26 + 'a');
		set3.push_back((i + 7) % 26 + 'a');
	}

	auto enq1 = async(&QTester::test_char_enqueuer,&tester,set1);  
	auto enq2 = async(&QTester::test_char_enqueuer,&tester,set2);  
	auto enq3 = async(&QTester::test_char_enqueuer,&tester,set3);  

	set1.insert(set1.end(),set2.begin(),set2.end());
	set1.insert(set1.end(),set3.begin(),set3.end());
	for(auto x: set1){
		cout << " " << x;
	}
	cout << endl;
	auto deq_thread = async(&QTester::test_char_dequeuer,&tester,set1);

	if( deq_thread.get() == false)
		rtv = false;
	
	return rtv;
}

int main() {
	int failures = 0;

	if(!basic_tests()) failures++;

	if(!thread_tests()) failures++;

	if(failures > 0) {
		cout << "Test failed: " << failures << endl;
	} else {
		cout << "All tests passed!" << endl;
	}

	return 0;
}
