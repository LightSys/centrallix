

== Caching / resizing-windows ==

This is a proposed model for allowing resizing of windows. The main
issue is that resizing the window requires the ***Apos unit*** to be
used, but the apos unit is on the server-side.

please note, if a better model is proposed, use it instead.

1) On server, cache every single generated WgtrNode.

      ~ Because all CSS positioning information sent back to client
        references targets by id (ie, "#wn15main {...}") (and to
        prevent having to send to server the id's of all affected
        widgets), this model has the server just cache every single
        WgtrNode instance because that contains all the correct id
        information.

      This is done (almost). The function nhtRenderApp is where the
      logic for caching is. There is a 'CachedApps' pXHash in the
      session variable now, but I do not _know_ if it will be
      something that automatically gets ***deallocated*** by the
      session de-allocator/deInit function.


2) on Client-side, have js code that will make a resizable black
   rectangle whenever the user clicks the corner of a child window.

      NOT implemented at time of writing.

3) When the user releases mouse button, a request is sent to server in
   the following format:
   "/INTERNAL/cache/#?cx__root=...&cx__onlyPositioning=1&cx__resize=..."

      NOT implemented at time of writing.

4) On server, when the URI is "/INTERNAL/cache/#" (where # is some number), then
   the cached value needs to be used

      This is NOT implemented at time of writing. Right now, the issue
      is that a restructering of control flow needs to prevent the
      normal calls to OSML (since "/INTERNAL/cache" is not a real
      item), but still:

      	   ~ go through nhtRenderApp (which DOES currently select the
      	     right cached app)

           ~ go through wgtrVerify (which has the apos unit)

5) Right before entering the apos unit, the server must look at
   information encoded into cx__resize param and use that information
   to find the appropriate height/width paramaters and change
   them.

      Right now, there is no poposed method of encoding the needed
      information.

6) htrRender must respond to the cx__onlyPositioning parameter by only
   running the logic which generates the CSS information.

      This will probably require refactoring all *Render functions (in
      htdrv_*.c files), to separate that logic into its own functions
      that can be used separately.

7) htrRender must respond to the cx__root parameter by starting the
   HTML generation at specified root.

8) When client recieves response, apply CSS.
