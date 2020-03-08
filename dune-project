(lang dune 1.11)
(name repack)
(version 0.1.0)
(source (github baransu/repack))
(license MIT)
(authors "baransu")
(maintainers "baransu")

(using fmt 1.2 (enabled_for reason))

(package
 (name repack)
 (synopsis "")
 (depends
  reason
  dune
  rely
  base
  cmdliner
  lwt
  console
  pastel))
