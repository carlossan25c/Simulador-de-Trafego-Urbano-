# Simulador de Tráfego Urbano em C

Trabalho da disciplina de **Sistemas Operacionais** da UFCA.

O objetivo é simular o tráfego de uma cidade em miniatura usando **threads** e **sincronização** em C. Cada veículo é uma thread que se move por um mapa de ruas, respeita semáforos e compete por espaço nas células da via.

## O que o projeto deve ter

- Entre 10 e 20 veículos rodando ao mesmo tempo, cada um como uma thread
- Uma ambulância com prioridade nos cruzamentos
- Semáforos em todos os cruzamentos, com veículos bloqueando no vermelho sem consumir CPU
- Pelo menos 8 cruzamentos no mapa, com ruas contínuas passando por eles
- Pelo menos uma via de mão única
- Prevenção de deadlock (nenhuma thread pode travar esperando por outra indefinidamente)
- Visualização em tempo real no terminal com caracteres ASCII

## Mapa

O mapa foi esboçado antes do desenvolvimento e está documentado em `docs/mapa_esboco.svg`.

![Esboço do mapa](docs/mapa_esboco.svg)

Layout definido: 23 linhas × 60 colunas, com 3 ruas horizontais e 4 verticais formando 12 cruzamentos. A linha 17 e a coluna 52 são vias de mão única.

## Estrutura do repositório

```
include/
  constants.h   — constantes globais acordadas pela equipe
  types.h       — structs compartilhadas por todos os módulos
src/            — código-fonte de cada módulo (desenvolvido individualmente)
tests/          — testes manuais de cada módulo
docs/           — esboços e documentação de planejamento
Makefile        — compilação do projeto
```

## Como compilar e rodar

```bash
make          # compila tudo que estiver em src/
./simulador   # executa a simulação
make clean    # remove os arquivos compilados
```

> Requer gcc e pthreads (padrão em Linux/WSL). Terminal com pelo menos 80×30 caracteres.

## Equipe

| Integrante | Responsabilidade | Branch |
|---|---|---|
| Carlos | _(a definir)_ | `feature/mapa-relogio` ou outra |
| David | _(a definir)_ | `feature/veiculos` ou outra |
| Henrique | _(a definir)_ | `feature/semaforos` ou outra |
| Levi | _(a definir)_ | `feature/render-integracao` ou outra |
