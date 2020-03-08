open Cmdliner;
open Repack;

module Ast = Flow_parser.Flow_ast;
module S = Ast.Statement;
module E = Ast.Expression;
module L = Ast.Literal;

type asset = {
  id: int,
  filename: string,
  code: string,
  dependencies: list(string),
  mapping: Base.Hashtbl.t(string, int),
};

let id = ref(-1);

let createAsset = filename => {
  let parse = source => {
    let parse_options =
      Some(
        Flow_parser.Parser_env.{
          esproposal_optional_chaining: false,
          esproposal_class_instance_fields: true,
          esproposal_class_static_fields: true,
          esproposal_decorators: true,
          esproposal_export_star_as: true,
          esproposal_nullish_coalescing: true,
          enums: true,
          types: true,
          types_in_comments: true,
          use_strict: false,
        },
      );
    Flow_parser.Parser_flow.program(source, ~parse_options);
  };

  Lwt_io.chars_of_file(filename)
  |> Lwt_stream.to_string
  |> Lwt.map(source => {
       let ((_, statements, _), errors) = parse(source);
       let dependencies = ref([]);
       statements
       |> List.map(~f=snd)
       |> List.iter(
            ~f=
              fun
              | S.ImportDeclaration({
                  importKind: ImportValue,
                  source: (_, Ast.StringLiteral.{value, _}),
                  _,
                }) =>
                dependencies := [value, ...dependencies^]
              | S.VariableDeclaration({declarations, _}) =>
                declarations
                |> List.map(~f=snd)
                |> List.iter(
                     ~f=
                       fun
                       | S.VariableDeclaration.Declarator.{
                           init:
                             Some((
                               _,
                               E.Call({
                                 callee: (
                                   _,
                                   E.Identifier((_, {name: "require"})),
                                 ),
                                 arguments: (
                                   _,
                                   [
                                     E.Expression((
                                       _,
                                       E.Literal({
                                         value: L.String(request),
                                         _,
                                       }),
                                     )),
                                   ],
                                 ),
                                 _,
                               }),
                             )),
                           _,
                         } =>
                         dependencies := [request, ...dependencies^]
                       | _ => (),
                   )

              | _ => (),
          );

       Int.incr(id);

       {
         id: id^,
         filename,
         dependencies: dependencies^,
         code: source,
         mapping: Base.Hashtbl.create((module String)),
       };
     })
  |> Lwt_main.run;
};

let transformPath = (~dirName, path) => {
  let path =
    if (path |> Base.String.is_suffix(~suffix=".js")) {
      path;
    } else {
      path ++ ".js";
    };

  Fp.At.(dirName / path) |> Fp.toDebugString;
};

let createGraph = entry => {
  let graph = ref([]);
  let mainAsset = createAsset(entry);
  let queue = Queue.create();
  Queue.enqueue(queue, mainAsset);

  let rec analyze = () => {
    switch (Base.Queue.dequeue(queue)) {
    | None => ()
    | Some(asset) =>
      graph := [asset, ...graph^];

      let dirName = Fp.relativeExn(asset.filename) |> Fp.dirName;

      asset.dependencies
      |> List.iter(~f=relativePath => {
           let child = createAsset(transformPath(~dirName, relativePath));
           Base.Hashtbl.set(asset.mapping, ~key=relativePath, ~data=child.id);
           Queue.enqueue(queue, child);

           analyze();
         });
    };
  };

  analyze();

  graph^;
};

let joinWith = (separator, collection) =>
  collection
  |> Base.List.reduce(~f=(acc, item) => acc ++ separator ++ item)
  |> Base.Option.value(~default="");

let bundle = graph => {
  let bundle = ref([]);

  graph
  |> List.iter(~f=m => {
       bundle :=
         [
           Int.to_string(m.id)
           ++ ": [function (require, module, exports) {"
           ++ m.code
           ++ "},{"
           ++ (
             m.mapping
             |> Base.Hashtbl.fold(~init=[], ~f=(~key, ~data, acc) =>
                  ["\"" ++ key ++ "\"" ++ ":" ++ Int.to_string(data), ...acc]
                )
             |> joinWith(",")
           )
           ++ "}]",
           ...bundle^,
         ]
     });

  let modules = bundle^ |> joinWith(",");

  {|
(function(modules) {
  function require(id) {
    const [fn, mapping] = modules[id];
    function localRequire(name) {
      return require(mapping[name]);
    }
    const module = { exports : {} };
    fn(localRequire, module, module.exports);
    return module.exports;
  }
  require(0);
  })({|}
  ++ modules
  ++ "})";
};

let run = (~entry) => {
  entry |> createGraph |> bundle |> Console.log;

  Lwt.return();
};

let cmd = {
  let doc = "Bundles stuff";

  let entry = {
    let doc = "Entry file";
    Arg.(
      required
      & pos(0, some(string), None)
      & info([], ~docv="ENTRY FILE", ~doc)
    );
  };

  let runCommand = entry => Lwt_main.run(run(~entry));

  (
    Term.(const(runCommand) $ entry),
    Term.info(
      "bundle",
      ~doc,
      ~envs=Man.envs,
      ~version=Man.version,
      ~exits=Man.exits,
      ~man=Man.man,
      ~sdocs=Man.sdocs,
    ),
  );
};
