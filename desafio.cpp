#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <omp.h>

using namespace std;
using namespace std::chrono;

// Estrutura para armazenar dados de uma pessoa
struct Pessoa {
    double estatura;
    double peso;
};

// Estrutura para representar uma classe de dados
struct Classe {
    double limite_inferior;
    double limite_superior;
    double ponto_medio;
    int frequencia;
    
    Classe(double li, double ls) : limite_inferior(li), limite_superior(ls) {
        ponto_medio = (li + ls) / 2.0;
        frequencia = 0;
    }
};

// Função para ler dados do arquivo
vector<Pessoa> lerDados(const string& nome_arquivo) {
    vector<Pessoa> dados;
    ifstream arquivo(nome_arquivo);
    
    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir o arquivo: " << nome_arquivo << endl;
        return dados;
    }
    
    Pessoa p;
    while (arquivo >> p.estatura >> p.peso) {
        dados.push_back(p);
    }
    
    arquivo.close();
    return dados;
}

// Função para criar classes
vector<Classe> criarClasses(double min_val, double max_val, double intervalo, const string& tipo) {
    vector<Classe> classes;
    
    double inicio = floor(min_val / intervalo) * intervalo;
    double fim = ceil(max_val / intervalo) * intervalo;
    
    for (double li = inicio; li < fim; li += intervalo) {
        double ls = li + intervalo;
        classes.push_back(Classe(li, ls));
    }
    
    cout << "Número de classes para " << tipo << ": " << classes.size() << endl;
    return classes;
}

// Função para calcular frequências (paralelizada)
void calcularFrequencias(vector<Classe>& classes, const vector<double>& dados) {
    #pragma omp parallel for
    for (size_t i = 0; i < classes.size(); i++) {
        int freq = 0;
        for (size_t j = 0; j < dados.size(); j++) {
            if (dados[j] >= classes[i].limite_inferior && dados[j] < classes[i].limite_superior) {
                freq++;
            }
        }
        classes[i].frequencia = freq;
    }
}

// Função para calcular média ponderada (paralelizada)
double calcularMediaPonderada(const vector<Classe>& classes, int N) {
    double soma_ponderada = 0.0;
    
    #pragma omp parallel for reduction(+:soma_ponderada)
    for (size_t i = 0; i < classes.size(); i++) {
        soma_ponderada += classes[i].ponto_medio * classes[i].frequencia;
    }
    
    return soma_ponderada / N;
}

// Função para calcular desvio padrão populacional (paralelizada)
double calcularDesvioPadrao(const vector<Classe>& classes, double media, int N) {
    double soma_quadrados = 0.0;
    
    #pragma omp parallel for reduction(+:soma_quadrados)
    for (size_t i = 0; i < classes.size(); i++) {
        double diferenca = classes[i].ponto_medio - media;
        soma_quadrados += classes[i].frequencia * (diferenca * diferenca);
    }
    
    return sqrt(soma_quadrados / N);
}

// Função para calcular coeficiente de variação
double calcularCoeficienteVariacao(double desvio_padrao, double media) {
    return (desvio_padrao / media) * 100.0;
}

// Função para processar uma variável (estatura ou peso)
void processarVariavel(const vector<double>& dados, double intervalo, const string& nome_variavel, 
                      double& media, double& desvio_padrao, double& cv) {
    
    // Encontrar mínimo e máximo
    double min_val = *min_element(dados.begin(), dados.end());
    double max_val = *max_element(dados.begin(), dados.end());
    
    cout << "\n=== Processando " << nome_variavel << " ===" << endl;
    cout << "Mínimo: " << min_val << ", Máximo: " << max_val << endl;
    
    // Criar classes
    vector<Classe> classes = criarClasses(min_val, max_val, intervalo, nome_variavel);
    
    // Calcular frequências
    calcularFrequencias(classes, dados);
    
    // Calcular estatísticas
    media = calcularMediaPonderada(classes, dados.size());
    desvio_padrao = calcularDesvioPadrao(classes, media, dados.size());
    cv = calcularCoeficienteVariacao(desvio_padrao, media);
    
    // Exibir resumo das classes
    cout << "\nResumo das classes:" << endl;
    for (size_t i = 0; i < min(classes.size(), size_t(5)); i++) {
        cout << "Classe " << i+1 << ": [" << classes[i].limite_inferior << ", " 
             << classes[i].limite_superior << ") - Frequência: " << classes[i].frequencia << endl;
    }
    if (classes.size() > 5) {
        cout << "... (" << classes.size() - 5 << " classes restantes)" << endl;
    }
}

int main() {
    string nome_arquivo = "dados.txt";
    
    // Ler dados
    cout << "Lendo dados do arquivo: " << nome_arquivo << endl;
    vector<Pessoa> dados = lerDados(nome_arquivo);
    
    if (dados.empty()) {
        cerr << "Nenhum dado foi lido. Verifique o arquivo." << endl;
        return 1;
    }
    
    cout << "Total de registros lidos: " << dados.size() << endl;
    
    // Separar dados de estatura e peso
    vector<double> estaturas, pesos;
    for (const auto& pessoa : dados) {
        estaturas.push_back(pessoa.estatura);
        pesos.push_back(pessoa.peso);
    }
    
    // Variáveis para resultados
    double media_estatura, desvio_estatura, cv_estatura;
    double media_peso, desvio_peso, cv_peso;
    
    // Medir tempo de execução sequencial
    auto inicio_seq = high_resolution_clock::now();
    
    // Processar estatura (sequencial)
    processarVariavel(estaturas, 8.0, "Estatura", media_estatura, desvio_estatura, cv_estatura);
    
    // Processar peso (sequencial)
    processarVariavel(pesos, 4.0, "Peso", media_peso, desvio_peso, cv_peso);
    
    auto fim_seq = high_resolution_clock::now();
    auto duracao_seq = duration_cast<milliseconds>(fim_seq - inicio_seq);
    
    // Medir tempo de execução paralelo
    auto inicio_par = high_resolution_clock::now();
    
    // Processar ambas as variáveis em paralelo
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            processarVariavel(estaturas, 8.0, "Estatura (Paralelo)", 
                            media_estatura, desvio_estatura, cv_estatura);
        }
        
        #pragma omp section
        {
            processarVariavel(pesos, 4.0, "Peso (Paralelo)", 
                            media_peso, desvio_peso, cv_peso);
        }
    }
    
    auto fim_par = high_resolution_clock::now();
    auto duracao_par = duration_cast<milliseconds>(fim_par - inicio_par);
    
    // Exibir resultados
    cout << "\n=== RESULTADOS FINAIS ===" << endl;
    cout << "\n--- ESTATURA ---" << endl;
    cout << "Média: " << media_estatura << " cm" << endl;
    cout << "Desvio Padrão: " << desvio_estatura << " cm" << endl;
    cout << "Coeficiente de Variação: " << cv_estatura << "%" << endl;
    
    cout << "\n--- PESO ---" << endl;
    cout << "Média: " << media_peso << " kg" << endl;
    cout << "Desvio Padrão: " << desvio_peso << " kg" << endl;
    cout << "Coeficiente de Variação: " << cv_peso << "%" << endl;
    
    // Análise de desempenho
    cout << "\n=== ANÁLISE DE DESEMPENHO ===" << endl;
    cout << "Tempo sequencial: " << duracao_seq.count() << " ms" << endl;
    cout << "Tempo paralelo: " << duracao_par.count() << " ms" << endl;
    cout << "Speedup: " << (double)duracao_seq.count() / duracao_par.count() << endl;
    cout << "Eficiência: " << ((double)duracao_seq.count() / duracao_par.count()) / omp_get_max_threads() * 100 << "%" << endl;
    
    return 0;
}
