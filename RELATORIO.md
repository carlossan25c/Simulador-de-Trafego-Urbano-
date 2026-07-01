# Relatório Técnico — Simulador de Tráfego Urbano em C

**Disciplina:** Sistemas Operacionais  
**Linguagem:** C | **Biblioteca:** Pthreads  
**Repositório:** https://github.com/carlossan25c/Simulador-de-Trafego-Urbano-

---

## Integrantes e Responsabilidades

As responsabilidades foram divididas igualmente entre os integrantes com auxílio de IA (Claude), que foi utilizada como ferramenta de apoio ao desenvolvimento — geração de código, revisão lógica e resolução de bugs de concorrência. A partir dessa divisão, o grupo se organizou com um board Kanban no GitHub Projects, adotando Git Flow: cada funcionalidade foi desenvolvida em uma branch própria (`feature/`, `fix/`), integrada à `develop` via Pull Request com revisão de código, e promovida à `main` apenas em versões estáveis.

| Nome | Módulo principal |
|------|-----------------|
| Levi | Mapa + Relógio Global (`map.c`, `clock.c`) |
| Carlos | Movimento dos Veículos + Deadlock (`vehicle.c`) |
| David | Semáforos + Ambulância (`traffic_light.c`, `ambulance.c`) |
| Henrique | Visualização ASCII + Integração (`render.c`, `main.c`) |

---

## 1. Mapa

O mapa é uma matriz estática de `26 × 60` células (`Cell grid[MAP_ROWS][MAP_COLS]`), onde cada célula armazena: tipo (`CELL_WALL`, `CELL_ROAD`, `CELL_INTERSECTION`), direção permitida (`DIR_EAST/WEST/NORTH/SOUTH`), flag de ocupação e um `pthread_mutex_t` individual.

**Layout:**

- **H1** (rows 5–6): via horizontal de mão dupla — row 5 = sentido leste (`>`), row 6 = sentido oeste (`<`)
- **H2** (rows 12–13): via horizontal de mão dupla — mesmo padrão
- **H3** (row 19): via horizontal de mão única, sentido leste
- **V1–V4** (colunas 10–11, 23–24, 36–37, 49–50): quatro vias verticais de mão dupla — coluna par = sul (`v`), coluna ímpar = norte (`^`)
- **8 cruzamentos** em blocos `2×2` nas interseções de H1 e H2 com V1–V4; H3 não recebe semáforo

O mutex por célula foi escolhido para minimizar contenção: cada veículo bloqueia apenas as duas células envolvidas no movimento, sem travar o mapa inteiro.

---

## 2. Threads

O programa cria **1 + 8 + 15 = 24 threads** no total:

| Thread | Quantidade | Criada em |
|--------|-----------|-----------|
| Relógio global | 1 | `spawn_clock()` em `main.c` |
| Semáforos | 8 (um por cruzamento) | `spawn_lights()` em `main.c` |
| Veículos (inclui ambulância) | 15 | `spawn_vehicles()` em `main.c` |

**Encerramento:** ao receber `SIGINT`, o handler seta a flag `g_running = 0` e faz `pthread_cond_broadcast` no relógio para desbloquear threads em espera. Cada thread de veículo checa `v->active` no topo do loop. A thread do relógio e as de semáforo são encerradas via `pthread_cancel`. O `main` aguarda todas com `pthread_join` antes de liberar recursos.

---

## 3. Mecanismos de Sincronização

### 3.1 Mutex

| Mutex | Onde | Protege |
|-------|------|---------|
| `cell->mutex` | `map.c` — `cell_lock/unlock` | ocupação e `vehicle_id` de cada célula |
| `tl->mutex` | `traffic_light.c` | `state_horizontal`, `state_vertical`, `elapsed_ticks` |
| `clk->mutex` | `clock.c` | contador `tick` |
| `log_mutex` | `render.c` | buffer de log de eventos |

Os locks de célula são sempre adquiridos em **ordem crescente de índice** (`row * MAP_COLS + col`) para garantir consistência entre threads que disputam células adjacentes.

### 3.2 Variáveis de Condição

**Relógio (`clock.c`):**
```c
// clock_thread: a cada TICK_MS ms, incrementa tick e acorda todos
pthread_cond_broadcast(&clk->cond);

// vehicle_thread / light_thread: dorme até o próximo tick
while (clk->tick == last_tick)
    pthread_cond_wait(&clk->cond, &clk->mutex);
```

**Semáforo (`traffic_light.c`):**
```c
// vehicle_thread: dorme enquanto sinal está vermelho
while (tl->state_horizontal == LIGHT_RED)
    pthread_cond_wait(&tl->cond, &tl->mutex);

// traffic_light_thread: acorda veículos ao trocar de fase
pthread_cond_broadcast(&tl->cond);
```

### 3.3 Semáforos POSIX

Não foram utilizados `sem_t`. Toda sincronização foi implementada com `pthread_mutex_t` + `pthread_cond_t`, que oferecem controle mais fino sobre as condições de espera.

---

## 4. Ausência de Espera Ocupada

Nenhuma thread usa loop ativo (`while(flag)` sem bloqueio). Os dois pontos críticos:

**Tick do relógio:** a thread de veículo chama `clock_wait_tick`, que faz `pthread_cond_wait` segurando o mutex — o SO a acorda apenas quando o relógio faz broadcast. Sem `sleep` ou polling.

**Semáforo vermelho:** ao encontrar sinal vermelho antes de entrar num cruzamento, o veículo chama `traffic_light_wait_green`, que bloqueia em `pthread_cond_wait` até a thread do semáforo trocar a fase e transmitir o broadcast. O veículo não consome CPU enquanto espera.

---

## 5. Estratégia contra Deadlock

**Problema:** dois veículos em células adjacentes tentando avançar simultaneamente podem entrar em deadlock clássico (cada um segura a célula atual e aguarda a do outro).

**Solução — ordenação global de locks:** antes de mover, o veículo calcula o índice linear das duas células envolvidas e adquire os mutexes sempre em ordem crescente:

```c
int cur_idx  = v->row * MAP_COLS + v->col;
int next_idx = nr     * MAP_COLS + nc;

if (cur_idx < next_idx) {
    cell_lock(map, v->row, v->col);
    cell_lock(map, nr, nc);
} else {
    cell_lock(map, nr, nc);
    cell_lock(map, v->row, v->col);
}
```

Essa ordenação elimina ciclos no grafo de recursos: se A precisa de (i, j) e B precisa de (j, i), ambos tentarão adquirir o de menor índice primeiro — um deles vence e avança, o outro aguarda sem segurar nada.

**Fallback — `stuck_ticks`:** se a célula destino estiver ocupada, o veículo libera ambos os locks e incrementa `stuck_ticks`. Após `MAX_STUCK_TICKS = 5` ticks consecutivos parado em um cruzamento, ele escolhe uma nova direção aleatória válida, quebrando engarrafamentos circulares.

---

## 6. Visualização ASCII

A renderização em `src/render.c`:

- **Sem piscar:** todo o frame é montado em um buffer de string interno e enviado ao terminal com um único `fwrite`, evitando flickering.
- **Leitura segura:** cada célula é lida sob `pthread_mutex_lock` (snapshot rápido), liberando o lock antes de processar o próximo caractere.
- **Semáforos sob lock:** estado de cada `TrafficLight` lido com mutex para evitar leitura de estado intermediário durante troca de fase.
- **Log thread-safe:** `render_log` usa `log_mutex` para permitir que qualquer thread registre eventos sem condição de corrida.

Símbolos utilizados:

| Elemento | Caractere | Cor |
|----------|-----------|-----|
| Parede | ` ` (espaço) | — |
| Via leste/oeste | `> <` | Branco |
| Via norte/sul | `^ v` | Branco |
| Cruzamento | `+` | Branco |
| Carro normal | `1–9` | Ciano |
| Ambulância | `A` | Vermelho brilhante |
| Semáforo verde | `G` | Verde |
| Semáforo vermelho | `R` | Vermelho |

---

## 7. Decisões de Implementação

**Mapa bidirecional com 2 células por faixa:** a abordagem de alocar uma coluna/linha por sentido de circulação eliminou toda a lógica de "qual direção é permitida nesta célula" — a própria posição da célula define o sentido, tornando o código de navegação mais simples.

**Navegação aleatória em vez de rotas fixas:** veículos seguem o sentido da faixa e, ao chegar a um cruzamento, têm 35% de chance de mudar para uma via perpendicular válida. Isso simula tráfego real sem necessidade de planejamento de rotas.

**Respawn em fim de via:** quando um veículo chega ao final de uma via (parede), ele desaparece e reaparece em um dos 13 pontos de entrada do mapa, simulando que o veículo saiu e entrou por uma área não representada.

**Ambulância com rota fixa e prioridade de semáforo:** a ambulância segue uma rota predefinida de waypoints e força o verde nos semáforos à frente — mas apenas quando a célula destino está livre, evitando um livelock onde ela impedia outros veículos de receber o verde de que precisavam.

---

## 8. Integração Final

A integração foi feita de forma incremental em `main.c`, seguindo a ordem de dependência dos módulos:

1. `map_init` → `clock_init` → `lights_init_all` (sem threads ainda)
2. `spawn_clock` → `spawn_lights` → `spawn_vehicles` (threads iniciadas nessa ordem)
3. Loop principal no `main`: aguarda cada tick via `clock_wait_tick` e chama `render_frame`
4. `SIGINT` seta `g_running = 0`, faz broadcast no relógio e aguarda todas as threads com `pthread_join`
5. Recursos liberados na ordem inversa: `traffic_light_destroy` → `clock_destroy` → `map_destroy` → `render_destroy`

Cada módulo foi desenvolvido e testado em branch isolada (`feature/mapa`, `feature/semaforos`, `feature/veiculos`, `feature/render-integracao`), integrado à `develop` via Pull Request com revisão, e promovido à `main` nas versões estáveis `v1.0.0` em diante.
