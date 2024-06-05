import desbordante as db

verificator = db.dc_verification.algorithms.Default()

verificator.load_data(table=("TestFD.csv", ',', True))

verificator.execute(denial_constraint="!(j.Col0 == s.Col0 and t.C <= t.B)")
