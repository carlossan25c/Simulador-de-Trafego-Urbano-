// Inicializa mapa stub (5x5, todas células livres)
// Inicializa clock stub
// Cria 3 Vehicle com rotas simples
// Lança 3 pthread_create com vehicle_thread
// Roda por 20 ticks
// A cada tick imprime: [tick N] VehicleID=X pos=(row,col)
// Ao final: pthread_join em todos
// Critério: nenhuma colisão (duas posições iguais), nenhum travamento