import desbordante

TABLE = 'datasets/theatres_mfd.csv'
METRIC = 'euclidean'
LHS_INDICES = [0]
RHS_INDICES = [2]
PARAMETER = 5

algo = desbordante.MetricVerifier()
algo.load_data(TABLE, ',', True)
algo.execute(lhs_indices=LHS_INDICES, metric=METRIC,
	     parameter=PARAMETER, rhs_indices=RHS_INDICES)
if algo.mfd_holds():
    print('MFD holds')
else:
    print('MFD not holds')
