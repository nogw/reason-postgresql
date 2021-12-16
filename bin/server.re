open Opium;

let ( let* ) = Lwt.bind;

let get_message_by_id = {
  App.get("/:id", req => {
    let id = Router.param(req, "id");
    let* message = Storage.select_message(id);
    message
    |> [%to_yojson: Storage.message]
    |> Response.of_json
    |> Response.set_status(`OK)
    |> Lwt.return;
  });
};

[@deriving yojson]
type message_list = list(Storage.message);

let get_all_messages = {
  App.get("/all", _ => {
    let* messages = Storage.select_all_messages();
    messages
    |> [%to_yojson: message_list]
    |> Response.of_json
    |> Response.set_status(`OK)
    |> Lwt.return;
  });
};

let post_message =
  App.post("/", req => {
    let* json_request = Request.to_json_exn(req);
    let json =
      switch (Storage.message_of_yojson(json_request)) {
      | Ok(message) => message
      | Error(error) => failwith(error)
      };
    let* id = Storage.insert_message(json);
    Response.of_json(
      `Assoc([("link", `String("http://localhost:3000/" ++ id))]),
    )
    |> Response.set_status(`Created)
    |> Lwt.return;
  });

let delete_message_by_id =
  App.delete("/:id", req => {
    let id = Router.param(req, "id");
    let* () = Storage.delete_message(id);
    Response.of_plain_text("DELETE " ++ id)
    |> Response.set_status(`OK)
    |> Lwt.return;
  });

let put_message =
  App.put("/", req => {
    let* json_request = Request.to_json_exn(req);

    let json =
      switch (Storage.message_to_update_of_yojson(json_request)) {
      | Ok(message) => message
      | Error(error) => failwith(error)
      };

    let* () = Storage.update_message(json);

    Response.of_plain_text("UPDATE " ++ json.id)
    |> Response.set_status(`OK)
    |> Lwt.return;
  });

let () = {
  Logs.set_reporter(Logs_fmt.reporter());
  Logs.set_level(Some(Logs.Error));

  switch (
    App.empty
    |> get_message_by_id
    |> get_all_messages
    |> post_message
    |> put_message
    |> delete_message_by_id
    |> App.run_command'
  ) {
  | `Error
  | `Not_running =>
    print_endline(<Pastel color=Red> "🟥 - Failed to start" </Pastel>)
  | `Ok(promise) =>
    print_endline(<Pastel color=Green> "✅ - Run on port 3000" </Pastel>);
    Lwt_main.run(promise);
  };
};
