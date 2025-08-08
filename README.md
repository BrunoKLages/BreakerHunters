# BreakerHunters

Projeto desenvolvido na competição Lean Challenge 2025, da empresa ABB Eletrificação em Contagem. O projeto visa implementar rastreabilidade dos produtos na linha, a partir de um sistema de verificação automática dos produtos (disjuntores)

A ESP32 recebe um parâmetro de produto via wifi e o identificador do produto via ethernet pelo modulo W5500. 
Além de conferir o resultado do teste a partir de uma api do MES.

Caso alguma das informações esteja conflitante, um relé é acionado e a esteira para.



Colaboradores:
Bruno Lages, Kaique Santos e Stephanny Moreira
