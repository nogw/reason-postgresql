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
    `Assoc([("link", `String("http://localhost:3000/" ++ id))])
    |> Response.of_json
    |> Response.set_status(`OK)
    |> Lwt.return;
  });

let delete_message =
  App.delete("/:id", req => {
    // TODO: returns a different response if the message does not exist

    let id = Router.param(req, "id");
    let.await _success = Storage.delete_message(id);
    Response.of_plain_text("DEL " ++ id)
    |> Response.set_status(`OK)
    |> Lwt.return;
  });

let put_message =
  // TODO: returns a different response of errors
  App.put("/", req => {
    let.await json_request = Request.to_json_exn(req);

    let json =
      switch (Storage.message_to_update_of_yojson(json_request)) {
      | Ok(message) => message
      | Error(error) => failwith(error)
      };

    let.await _success = Storage.update_message(json);
    Response.of_plain_text("UPDATE " ++ json.id)
    |> Response.set_status(`OK)
    |> Lwt.return;
  });

let () =
  App.empty
  |> get_message
  |> post_message
  |> put_message
  |> delete_message
  |> App.run_command
  |> ignore;
