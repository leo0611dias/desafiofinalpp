#include <iostream>
#include <fstream>
#include <random>
#include <vector>

using namespace std;

int main() {
    const int NUM_PESSOAS = 1000;
    ofstream arquivo("dados.txt");
    
    random_device rd;
    mt19937 gen(rd());
    
    // Distribuição para estatura (cm): média 170, desvio padrão 10
    normal_distribution<double> estatura_dist(170.0, 10.0);
    
    // Distribuição para peso (kg): média 70, desvio padrão 15
    normal_distribution<double> peso_dist(70.0, 15.0);
    
    for (int i = 0; i < NUM_PESSOAS; i++) {
        double estatura = estatura_dist(gen);
        double peso = peso_dist(gen);
        
        // Garantir valores realisticos
        estatura = max(150.0, min(estatura, 200.0));
        peso = max(40.0, min(peso, 120.0));
        
        arquivo << estatura << " " << peso << endl;
    }
    
    arquivo.close();
    cout << "Arquivo 'dados.txt' gerado com " << NUM_PESSOAS << " registros." << endl;
    
    return 0;
}
