open Opium;

let (let.await) = Lwt.bind;

let get_message = {
  App.get("/:id", req => {
    let id = Router.param(req, "id");
    let.await message = Storage.select_message(id);
    message
    |> [%to_yojson: Storage.message]
    |> Response.of_json
    |> Response.set_status(`OK)
    |> Lwt.return;
  });
};

let post_message =
  App.post("/", req => {
    let.await json_request = Request.to_json_exn(req);
    let json =
      switch (Storage.message_of_yojson(json_request)) {
      | Ok(message) => message
      | Error(error) => failwith(error)
      };
    let.await id = Storage.insert_message(json);
    Response.of_json(
      `Assoc([("link", `String("http://localhost:3000/" ++ id))]),
    )
    |> Response.set_status(`Created)
    |> Lwt.return;
  });

let delete_message =
  App.delete("/:id", req => {
    let id = Router.param(req, "id");
    let.await success = Storage.delete_message(id);

    Response.of_plain_text("DEL " ++ id)
    |> Response.set_status(`OK)
    |> Lwt.return;
  });

let delete_message =
  App.delete("/:id", req => {
    let id = Router.param(req, "id");
    let.await () = Storage.delete_message(id);
    Response.of_plain_text("DELETE " ++ id)
    |> Response.set_status(`OK)
    |> Lwt.return;
  });

let put_message =
  App.put("/", req => {
    let.await json_request = Request.to_json_exn(req);

    let json =
      switch (Storage.message_to_update_of_yojson(json_request)) {
      | Ok(message) => message
      | Error(error) => failwith(error)
      };

    let.await () = Storage.update_message(json);

    Response.of_plain_text("UPDATE " ++ json.id)
    |> Response.set_status(`OK)
    |> Lwt.return;
  });

let () = {
  Logs.set_reporter(Logs_fmt.reporter());
  Logs.set_level(Some(Logs.Error));

  switch (
    App.empty
    |> get_message
    |> post_message
    |> put_message
    |> delete_message
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
