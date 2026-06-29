# Relatório Técnico — Simulador de Tráfego Urbano em C

**Disciplina:** Sistemas Operacionais  
**Linguagem:** C | **Biblioteca:** Pthreads  
**Repositório:** https://github.com/carlossan25c/Simulador-de-Trafego-Urbano-

---

## Integrantes e Responsabilidades

| Nome | Responsabilidade |
|------|-----------------|
| Levi | Mapa + Relógio Global (`map.c`, `clock.c`) |
| Carlos | Movimento dos Veículos + Deadlock (`vehicle.c`) |
| David | Semáforos + Ambulância (`traffic_light.c`, `ambulance.c`) |
| Henrique | Visualização ASCII + Integração (`render.c`, `main.c`) |

---

## 1. Mapa

> *(Levi preenche aqui)*
>
> Descrever: layout escolhido, número de cruzamentos, onde fica a via de mão
> única, como as células são representadas na matriz, por que foi escolhido
> mutex por célula.

---

## 2. Threads

> *(Todos contribuem aqui)*
>
> Descrever: quantas threads existem no total, quem cria cada uma, como são
> encerradas ao fim da simulação (SIGINT / flag global).

---

## 3. Mecanismos de Sincronização

### 3.1 Mutex

> *(Carlos e David preenchem os trechos das suas partes)*
>
> Descrever onde cada mutex é usado e por quê. Exemplos esperados:
> - Mutex por célula do mapa (exclusão mútua de ocupação)
> - Mutex do semáforo (proteção de `state_horizontal` / `state_vertical`)
> - Mutex do relógio (proteção do contador de tick)
> - Mutex interno do log de renderização

### 3.2 Variáveis de Condição

> *(Levi e David preenchem)*
>
> Descrever onde `pthread_cond_wait` e `pthread_cond_broadcast` são usados.
> Exemplos esperados:
> - Relógio: `cond_broadcast` acorda todos os veículos a cada tick
> - Semáforo: veículo chama `cond_wait` enquanto sinal está vermelho

### 3.3 Semáforos POSIX

> *(preencher se `sem_t` foi usado em algum módulo)*
>
> Descrever onde semáforos POSIX foram aplicados, se houver.

---

## 4. Ausência de Espera Ocupada

> *(Levi e Carlos preenchem)*
>
> Explicar como o relógio acorda as threads com `cond_broadcast` em vez de
> as threads ficarem em loop testando o tempo. Explicar como os veículos
> dormem no vermelho com `cond_wait` em vez de loop ativo. Citar os trechos
> de código relevantes.

---

## 5. Estratégia contra Deadlock

> *(Carlos preenche)*
>
> Descrever a ordem fixa de aquisição de locks adotada (ordem crescente de
> `row * MAP_COLS + col`). Explicar onde ela é aplicada no código. Argumentar
> por que essa estratégia previne deadlock (grafo de recursos sem ciclo).
> Descrever o que acontece quando um veículo não consegue a segunda célula
> (libera a primeira e tenta no próximo tick).

---

## 6. Visualização ASCII

A renderização foi implementada em `src/render.c` com as seguintes
características:

- **Sem piscar:** todo o frame é montado em um buffer de string e enviado
  ao terminal com um único `fwrite`, evitando flickering.
- **Leitura segura:** cada célula do mapa é lida sob `pthread_mutex_lock`
  antes de ser desenhada, copiando apenas os campos necessários e liberando
  o lock imediatamente.
- **Semáforos sob lock:** o estado de cada `TrafficLight` é lido com
  `pthread_mutex_lock` para evitar leitura de estado intermediário durante
  troca de fase.
- **Log thread-safe:** `render_log` usa mutex interno para permitir que
  qualquer thread (ambulância, semáforo) registre eventos sem condição de
  corrida.

Símbolos utilizados:

| Elemento | Caractere | Cor |
|----------|-----------|-----|
| Parede | `#` | Cinza |
| Via horizontal | `─` | Branco |
| Via vertical | `│` | Branco |
| Cruzamento | `+` | Branco |
| Via de mão única | `→ ↑ ↓ ←` | Amarelo |
| Carro normal | `0–9` | Ciano |
| Ambulância | `A` | Vermelho brilhante |
| Semáforo verde | `G` | Verde |
| Semáforo vermelho | `R` | Vermelho |

---

## 7. Decisões de Implementação

> *(Todos contribuem com um parágrafo cada)*
>
> Descrever o que foi mais difícil, o que foi simplificado e por quê.

---

## 8. Integração Final

> *(Henrique preenche após a integração no `main.c`)*
>
> Descrever as etapas de integração progressiva, ordem dos merges, como o
> encerramento limpo foi implementado (handler de SIGINT, `pthread_join`
> em todas as threads).