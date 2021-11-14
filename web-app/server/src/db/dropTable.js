async function dropTableTasks(pool) {
    return await pool.query(`DROP TABLE IF EXISTS ${process.env.DB_TASKS_TABLE_NAME}`)
    .then((res) => {
        console.log(`Tables in DB '${process.env.DB_NAME}' was successfully dropped`)
    })
    .catch((err) => {
            console.log(`Problem with dropping table ${process.env.DB_TASKS_TABLE_NAME}`)
            console.log(err);
            throw err
        }
    )
};

module.exports = dropTableTasks;