#ifndef SIMULATION_H
#define SIMULATION_H

// Simulation update functions
void UpdateGrid(void);
void UpdateSoil(void);
void UpdateWater(void);
void UpdateVapor(void);
void TransferMoisture(void);

// Helper functions
void FloodFillCeilingCluster(int x, int y, int clusterID, int** clusterIDs);
int CountWaterNeighbors(int x, int y);

#endif // SIMULATION_H
