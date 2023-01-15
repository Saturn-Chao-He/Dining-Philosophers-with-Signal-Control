### Dining Philosophers with Signal Control

### Program name: Dining Philosopher with signal control
### Programmer: Chao He 
### g++ dp.cpp -o dp -lpthread
### You need to open 6 terminals:
### terminal 1: ./dp 0 
### terminal 2: ./dp 1 
### terminal 3: ./dp 2 
### terminal 4: ./dp 3 
### terminal 5: ./dp 4 
### terminal 6: send the start sinal using kill -n 30 [PID], send the stop sinal using kill -n 31 [PID]

### This is a simulation of distributed system that has 5 machines share 5 resources in a decentralized method. 
### This exemple used one Macbook run 6  terminals. 
### The 5 IPs are same, but the 5 ports are different. If you need to run on 5 different machines, just modify the 5 IP address.


![Screen Shot 2023-01-15 at 23 22 01](https://user-images.githubusercontent.com/56700281/212550977-bb04d726-8e23-4a5a-8612-ae74be147009.png)
