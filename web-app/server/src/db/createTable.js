async function createTable(pool) {
    console.log(`Creating table(-es) in DB '${process.env.DB_TASKS_TABLE_NAME}'`)

    // status -- ADDED TO THE TASK QUEUE/IN PROCESS/COMPLETED/INCORRECT INPUT DATA/SERVER ERROR
    return await pool.query(
        `CREATE TABLE IF NOT EXISTS ${process.env.DB_TASKS_TABLE_NAME}(\n
        taskID char(40) not null primary key,\n
        createdAt timestamp not null,\n
        algName char(10) not null,\n 
        errorPercent real not null,\n
        separator char(1) not null,\n
        progress real not null,\n
        currentPhase int,\n
        maxPhase int,\n
        phaseName text,\n
        elapsedTime bigint CHECK (elapsedTime >= 0),\n
        status varchar(30) not null,\n
        errorStatus text,\n
        datasetPath text not null,\n
        fileName text not null,
        FDs text,\n
        hasHeader bool not null,\n
        maxLHS int not null,\n
        parallelism int not null,\n
        cancelled bool not null,\n
        arrayNameValue text,\n
        columnNames text)
        `
    )
    .then(res => {
        if (res !== undefined) 
            console.log(`Table '${process.env.DB_TASKS_TABLE_NAME}' was successfully created.`)
        }
    )
    .catch(err => {
        console.log('Error with table creation')
        throw err
    })
}

module.exports = createTable;