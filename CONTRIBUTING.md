# Como contribuir

## Formato de commit

```
[módulo] descrição curta no imperativo
```

Exemplos:
```
[map] inicializa a matriz com layout do mapa
[clock] implementa espera por tick com cond_wait
[vehicle] adiciona lógica de movimento por tick
[semaforo] implementa bloqueio no sinal vermelho
[render] desenha mapa no terminal em ASCII
[main] integra threads de veículos
```

## Branches

Cada pessoa trabalha na sua branch e abre Pull Request para `develop`:

| Integrante | Branch |
|---|---|
| Pessoa 1 (mapa + relógio) | `feature/mapa-relogio` |
| Pessoa 2 (veículos) | `feature/veiculos` |
| Pessoa 3 (semáforos + ambulância) | `feature/semaforos` |
| Pessoa 4 (renderização + integração) | `feature/render-integracao` |
