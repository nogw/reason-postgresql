let connection_pg_url = "postgresql://postgres:postgres@localhost:5432";

exception Query_Error(string);

let (let.await) = Lwt.bind;

let pool =
  switch (
    Caqti_lwt.connect_pool(~max_size=10, Uri.of_string(connection_pg_url))
  ) {
  | Ok(pool) => pool
  | Error(error) => failwith(Caqti_error.show(error))
  };

let dispatch = f => {
  let.await result = Caqti_lwt.Pool.use(f, pool);

  switch (result) {
  | Ok(data) => Lwt.return(data)
  | Error(error) => Lwt.fail(Query_Error(Caqti_error.show(error)))
  };
};

[@deriving yojson]
type message = {
  name: string,
  message: string,
};

[@deriving yojson]
type message_stored = {
  id: string,
  name: string,
  message: string,
};

[@deriving yojson]
type message_to_update = {
  id: string,
  message: string,
};

let ensure_table_exists = [%rapper
  execute(
    {sql|
      CREATE TABLE IF NOT EXISTS messages (
        id uuid PRIMARY KEY NOT NULL,
        name VARCHAR,
        message VARCHAR
      )
    |sql},
  )
];

let insert =
  [%rapper
    get_one(
      {sql|
        INSERT INTO messages (id, name, message)
        VALUES (%string{id}, %string{name}, %string{message})
        RETURNING @string{id}
      |sql},
      record_in,
      function_out,
    )
  ](
    (~id) =>
    id
  );

let select = [%rapper
  get_one(
    {sql|
      SELECT @string{name}, @string{message}
      FROM messages
      WHERE id = %string{id}
    |sql},
    record_out,
  )
];

let update = [%rapper
  execute(
    {sql|
      UPDATE messages
      SET message = %string{message}
      WHERE id = %string{id}
    |sql},
    record_in,
  )
];

let delete = [%rapper
  execute(
    {sql|
      DELETE FROM messages
      WHERE id = %string{id}
    |sql},
  )
];

let insert_message = ({name, message}: message) => {
  let id = Uuidm.create(`V4) |> Uuidm.to_string;
  let.await message = dispatch(insert({id, name, message}));
  message |> Lwt.return;
};

let select_message = id => {
  let.await message = dispatch(select(~id));
  message |> Lwt.return;
};

let update_message = ({id, message}: message_to_update) => {
  let.await update = dispatch(update({id, message}));
  update |> Lwt.return;
};

let delete_message = id => {
  let.await delete = dispatch(delete(~id));
  delete |> Lwt.return;
};

let () = dispatch(ensure_table_exists()) |> Lwt_main.run;
