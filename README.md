# Simulador de Tráfego Urbano em C

Trabalho da disciplina de **Sistemas Operacionais** da UFCA.

Simulação concorrente de tráfego urbano em C, na qual veículos são representados por threads e competem por espaços de uma malha viária. O projeto demonstra na prática os conceitos de concorrência, exclusão mútua, espera bloqueante, variáveis de condição e prevenção de deadlocks.

Repositório: https://github.com/carlossan25c/Simulador-de-Trafego-Urbano-

---

## Dependências

- gcc
- pthreads
- Terminal com suporte a cores ANSI e pelo menos 80×30 caracteres

---

## Como compilar e rodar

```bash
make          # compila todos os arquivos em src/
./simulador   # executa a simulação
make clean    # remove arquivos compilados
```

---

## Mapa

Grade de **23 linhas × 60 colunas** com 12 cruzamentos formados por 3 ruas horizontais e 4 verticais.

```
     col: 0         1         2         3         4         5
          0123456789012345678901234567890123456789012345678901234567890
  row  5:  ---------+-----------+-----------+-----------+-----------
  row 11:  ---------+-----------+-----------+-----------+-----------
  row 17:  >>>>>>>>>+>>>>>>>>>>>+>>>>>>>>>>>+>>>>>>>>>>>+>>>>>>>>>>>
```

- Ruas horizontais de **mão dupla**: linhas 5 e 11
- Rua horizontal de **mão única** (→ leste): linha 17
- Ruas verticais de **mão dupla**: colunas 10, 22, 34 e 46
- 12 cruzamentos nas interseções

O esboço visual do mapa está em [`docs/Mapa.png`](docs/Mapa.png).

---

## Estrutura do repositório

```
include/
  constants.h       — constantes globais (dimensões, velocidades, IDs, tipos de célula)
  types.h           — structs compartilhadas (Cell, Map, Vehicle, TrafficLight, SimClock)
  map.h             — interface do módulo de mapa
  clock.h           — interface do relógio global
  vehicle.h         — interface do módulo de veículos
  traffic_light.h   — interface dos semáforos
  ambulance.h       — interface da ambulância
  render.h          — interface da visualização ASCII
src/
  map.c             — inicialização do mapa e funções de célula
  clock.c           — relógio global com cond_broadcast por tick
  vehicle.c         — thread de veículo, movimento e prevenção de deadlock
  traffic_light.c   — thread de semáforo e bloqueio por variável de condição
  ambulance.c       — thread da ambulância com prioridade em cruzamentos
  render.c          — visualização ASCII do estado da simulação
  main.c            — integração de todos os módulos (em desenvolvimento)
tests/
  test_map.c        — imprime o mapa estático em ASCII
  test_clock.c      — valida 10 ticks em intervalos regulares
  test_vehicle.c    — 3 veículos por 20 ticks sem colisão
  test_traffic_light.c — veículo bloqueia no vermelho e é acordado no verde
  stubs/            — implementações falsas para testes isolados
docs/
  Mapa.png          — esboço visual do layout da malha
Makefile
RELATORIO.md
```

---

## Uso de Inteligência Artificial

Durante o desenvolvimento deste projeto, a equipe utilizou o **Claude (Anthropic)** como ferramenta auxiliar em algumas etapas do processo, como:

- Apoio na divisão e organização das tarefas entre os integrantes
- Esclarecimento de dúvidas pontuais sobre  organização do código e alguns bugs em C
- Revisão de trechos de código durante o desenvolvimento
- Organização deste README e do RELATORIO.md

A equipe foi **autogerenciada** em todas as etapas: as decisões de arquitetura, implementação e integração foram tomadas pelos próprios integrantes. Todo o código foi escrito, revisado e validado pela equipe. A IA foi utilizada como uma ferramenta de apoio — para reduzir trabalho repetitivo, tirar dúvidas e acelerar tarefas de organização — mas **nunca como substituta do julgamento técnico da equipe**.

---

## Equipe

| Integrante | Responsabilidade 
|---|---|
| Levi | Mapa + Relógio global + Revisão Geral|
| Carlos | Movimento dos veículos + Prevenção de deadlock |
| David | Semáforos de trânsito + Ambulância |
| Henrique | Visualização ASCII + Integração + Relatório |
