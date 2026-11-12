---
scope:
  - "**/*.app"
  - "**/*.cmp"
  - "**/*.tpl"
---
Many widget types are valid only inside a specific parent, and many parents accept only specific children. When a PR adds or moves a widget, check that its placement is supported, and flag a widget placed where no driver handles it: these usually fail silently. Where to check:
- The parent's drivers. A wgtr driver (`centrallix/wgtr/wgtdrv_<name>.c`) that registers extra types with `wgtrAddType()` alongside its own usually handles them as children in its HTML driver (`centrallix/htmlgen/htdrv_<name>.c`). For example, `widget/component-decl-action` must be a direct child of `widget/component-decl`, which renders it inside its own loop; anywhere else it is silently ignored. Others include `widget/tabpage` in `widget/tab`, `widget/menuitem` in `widget/menu`, `widget/osrc-rule` in `widget/osrc`, and `widget/table-column` in `widget/table`.
- `centrallix-doc/Widgets/widgets.xml`, which documents where each widget "can be placed" and what it "may only contain". For example, `widget/parameter` is valid only inside `widget/page`, `widget/component-decl`, or `widget/osrc`, and a `widget/component` instance may contain only connectors.
