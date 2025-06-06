import { addMethods, settings, nsREX, isFunc } from '../core.mjs';
import { select as d3_select, rgb as d3_rgb, pointer as d3_pointer } from '../d3.mjs';
import { makeTranslate } from '../base/BasePainter.mjs';
import { RObjectPainter } from '../base/RObjectPainter.mjs';
import { addDragHandler } from '../gpad/TFramePainter.mjs';
import { ensureRCanvas } from '../gpad/RCanvasPainter.mjs';
import { createMenu } from '../gui/menu.mjs';


/** @summary draw RText object
  * @private */
function drawText() {
   const text = this.getObject(),
       pp = this.getPadPainter(),
       onframe = this.v7EvalAttr('onFrame', false) ? pp.getFramePainter() : null,
       clipping = onframe ? this.v7EvalAttr('clipping', false) : false,
       p = pp.getCoordinate(text.fPos, onframe),
       textFont = this.v7EvalFont('text', { size: 12, color: 'black', align: 22 });

   this.createG(clipping ? 'main_layer' : (onframe ? 'upper_layer' : false));

   return this.startTextDrawingAsync(textFont, 'font').then(() => {
      this.drawText({ x: p.x, y: p.y, text: text.fText, latex: 1 });
      return this.finishTextDrawing();
   });
}

/** @summary draw RLine object
  * @private */
function drawLine() {
    const line = this.getObject(),
        pp = this.getPadPainter(),
        onframe = this.v7EvalAttr('onFrame', false) ? pp.getFramePainter() : null,
        clipping = onframe ? this.v7EvalAttr('clipping', false) : false,
        p1 = pp.getCoordinate(line.fP1, onframe),
        p2 = pp.getCoordinate(line.fP2, onframe);

    this.createG(clipping ? 'main_layer' : (onframe ? 'upper_layer' : false));

    this.createv7AttLine();

    this.draw_g
        .append('svg:path')
        .attr('d', `M${p1.x},${p1.y}L${p2.x},${p2.y}`)
        .call(this.lineatt.func);
}

/** @summary draw RBox object
  * @private */
function drawBox() {
   const box = this.getObject(),
         pp = this.getPadPainter(),
         onframe = this.v7EvalAttr('onFrame', false) ? pp.getFramePainter() : null,
         clipping = onframe ? this.v7EvalAttr('clipping', false) : false,
         p1 = pp.getCoordinate(box.fP1, onframe),
         p2 = pp.getCoordinate(box.fP2, onframe);

   this.createG(clipping ? 'main_layer' : (onframe ? 'upper_layer' : false));

   this.createv7AttLine('border_');

   this.createv7AttFill();

   this.draw_g
       .append('svg:path')
       .attr('d', `M${p1.x},${p1.y}H${p2.x}V${p2.y}H${p1.x}Z`)
       .call(this.lineatt.func)
       .call(this.fillatt.func);
}

/** @summary draw RMarker object
  * @private */
function drawMarker() {
    const marker = this.getObject(),
          pp = this.getPadPainter(),
          onframe = this.v7EvalAttr('onFrame', false) ? pp.getFramePainter() : null,
          clipping = onframe ? this.v7EvalAttr('clipping', false) : false,
          p = pp.getCoordinate(marker.fP, onframe);

    this.createG(clipping ? 'main_layer' : (onframe ? 'upper_layer' : false));

    this.createv7AttMarker();

    const path = this.markeratt.create(p.x, p.y);

    if (path) {
       this.draw_g.append('svg:path')
                  .attr('d', path)
                  .call(this.markeratt.func);
    }
}

/** @summary painter for RPalette
 *
 * @private
 */

class RPalettePainter extends RObjectPainter {

   /** @summary get palette */
   getHistPalette() {
      const pal = this.getObject()?.fPalette;

      if (pal && !isFunc(pal.getColor))
         addMethods(pal, `${nsREX}RPalette`);

      return pal;
   }

   /** @summary Draw palette */
   drawPalette(drag) {
      const palette = this.getHistPalette(),
            contour = palette.getContour(),
            fp = this.getFramePainter();

      if (!contour)
         return console.log('no contour - no palette');

      // frame painter must  be there
      if (!fp)
         return console.log('no frame painter - no palette');

      const zmin = contour.at(0),
            zmax = contour.at(-1),
            rect = fp.getFrameRect(),
            pad_width = this.getPadPainter().getPadWidth(),
            pad_height = this.getPadPainter().getPadHeight(),
            visible = this.v7EvalAttr('visible', true),
            vertical = this.v7EvalAttr('vertical', true);
      let gmin = palette.full_min,
          gmax = palette.full_max,
          palette_x, palette_y, palette_width, palette_height;

      if (drag) {
         palette_width = drag.width;
         palette_height = drag.height;

         const changes = {};
         if (vertical) {
            this.v7AttrChange(changes, 'margin', (drag.x - rect.x - rect.width) / pad_width);
            this.v7AttrChange(changes, 'width', palette_width / pad_width);
         } else {
            this.v7AttrChange(changes, 'margin', (drag.y - rect.y - rect.height) / pad_width);
            this.v7AttrChange(changes, 'width', palette_height / pad_height);
         }
         this.v7SendAttrChanges(changes, false); // do not invoke canvas update on the server
      } else {
          if (vertical) {
            const margin = this.v7EvalLength('margin', pad_width, 0.02);
            palette_x = Math.round(rect.x + rect.width + margin);
            palette_width = this.v7EvalLength('width', pad_width, 0.05);
            palette_y = rect.y;
            palette_height = rect.height;
          } else {
            const margin = this.v7EvalLength('margin', pad_height, 0.02);
            palette_x = rect.x;
            palette_width = rect.width;
            palette_y = Math.round(rect.y + rect.height + margin);
            palette_height = this.v7EvalLength('width', pad_height, 0.05);
          }

          // x,y,width,height attributes used for drag functionality
          makeTranslate(this.draw_g, palette_x, palette_y);
      }

      let g_btns = this.draw_g.selectChild('.colbtns');
      if (g_btns.empty())
         g_btns = this.draw_g.append('svg:g').attr('class', 'colbtns');
      else
         g_btns.selectAll('*').remove();

      if (!visible) return;

      g_btns.append('svg:path')
          .attr('d', `M0,0H${palette_width}V${palette_height}H0Z`)
          .style('stroke', 'black')
          .style('fill', 'none');

      if ((gmin === undefined) || (gmax === undefined)) { gmin = zmin; gmax = zmax; }

      if (vertical)
         fp.z_handle.configureAxis('zaxis', gmin, gmax, zmin, zmax, true, [palette_height, 0], -palette_height, { reverse: false });
      else
         fp.z_handle.configureAxis('zaxis', gmin, gmax, zmin, zmax, false, [0, palette_width], palette_width, { reverse: false });

      for (let i = 0; i < contour.length - 1; ++i) {
         const z0 = Math.round(fp.z_handle.gr(contour[i])),
               z1 = Math.round(fp.z_handle.gr(contour[i+1])),
               col = palette.getContourColor((contour[i] + contour[i+1]) / 2),

         r = g_btns.append('svg:path')
                   .attr('d', vertical ? `M0,${z1}H${palette_width}V${z0}H0Z` : `M${z0},0V${palette_height}H${z1}V0Z`)
                   .style('fill', col)
                   .style('stroke', col)
                   .property('fill0', col)
                   .property('fill1', d3_rgb(col).darker(0.5).formatRgb());

         if (this.isBatchMode())
            continue;

         if (this.isTooltipAllowed()) {
            r.on('mouseover', function() {
               d3_select(this).transition().duration(100).style('fill', d3_select(this).property('fill1'));
            }).on('mouseout', function() {
               d3_select(this).transition().duration(100).style('fill', d3_select(this).property('fill0'));
            }).append('svg:title').text(contour[i].toFixed(2) + ' - ' + contour[i+1].toFixed(2));
         }

         if (settings.Zooming)
            r.on('dblclick', () => fp.unzoom('z'));
      }

      fp.z_handle.maxTickSize = Math.round(palette_width*0.3);

      const promise = fp.z_handle.drawAxis(this.draw_g, makeTranslate(vertical ? palette_width : 0, palette_height), vertical ? -1 : 1);

      if (this.isBatchMode() || drag)
         return promise;

      return promise.then(() => {
         if (settings.ContextMenu) {
            this.draw_g.on('contextmenu', evnt => {
               evnt.stopPropagation(); // disable main context menu
               evnt.preventDefault();  // disable browser context menu
               createMenu(evnt, this).then(menu => {
                  menu.header('Palette');
                  menu.addchk(vertical, 'Vertical', flag => { this.v7SetAttr('vertical', flag); this.redrawPad(); });
                  fp.z_handle.fillAxisContextMenu(menu, 'z');
                  menu.show();
               });
            });
         }

         addDragHandler(this, { x: palette_x, y: palette_y, width: palette_width, height: palette_height,
                                minwidth: 20, minheight: 20, no_change_x: !vertical, no_change_y: vertical, redraw: d => this.drawPalette(d) });

         if (!settings.Zooming) return;

         let doing_zoom = false, sel1 = 0, sel2 = 0, zoom_rect, zoom_rect_visible, moving_labels, last_pos;

         const moveRectSel = evnt => {
            if (!doing_zoom) return;
            evnt.preventDefault();

            last_pos = d3_pointer(evnt, this.draw_g.node());

            if (moving_labels)
               return fp.z_handle.processLabelsMove('move', last_pos);

            if (vertical)
               sel2 = Math.min(Math.max(last_pos[1], 0), palette_height);
            else
               sel2 = Math.min(Math.max(last_pos[0], 0), palette_width);

            const sz = Math.abs(sel2-sel1);

            if (!zoom_rect_visible && (sz > 1)) {
               zoom_rect.style('display', null);
               zoom_rect_visible = true;
            }

            if (vertical)
               zoom_rect.attr('y', Math.min(sel1, sel2)).attr('height', sz);
            else
               zoom_rect.attr('x', Math.min(sel1, sel2)).attr('width', sz);
         }, endRectSel = evnt => {
            if (!doing_zoom) return;

            evnt.preventDefault();
            d3_select(window).on('mousemove.colzoomRect', null)
                             .on('mouseup.colzoomRect', null);
            zoom_rect.remove();
            zoom_rect = null;
            doing_zoom = false;

            if (moving_labels)
               fp.z_handle.processLabelsMove('stop', last_pos);
             else {
               const z = fp.z_handle.func, z1 = z.invert(sel1), z2 = z.invert(sel2);
               fp.zoomSingle('z', Math.min(z1, z2), Math.max(z1, z2));
            }
         }, startRectSel = evnt => {
            // ignore when touch selection is activated
            if (doing_zoom) return;
            doing_zoom = true;

            evnt.preventDefault();
            evnt.stopPropagation();

            last_pos = d3_pointer(evnt, this.draw_g.node());
            sel1 = sel2 = last_pos[vertical ? 1 : 0];
            zoom_rect_visible = false;
            moving_labels = false;
            zoom_rect = g_btns
                 .append('svg:rect')
                 .attr('class', 'zoom')
                 .attr('id', 'colzoomRect')
                 .style('display', 'none');
            if (vertical)
               zoom_rect.attr('x', 0).attr('width', palette_width).attr('y', sel1).attr('height', 1);
            else
               zoom_rect.attr('x', sel1).attr('width', 1).attr('y', 0).attr('height', palette_height);

            d3_select(window).on('mousemove.colzoomRect', moveRectSel)
                             .on('mouseup.colzoomRect', endRectSel, true);

            setTimeout(() => {
               if (!zoom_rect_visible && doing_zoom)
                  moving_labels = fp.z_handle.processLabelsMove('start', last_pos);
            }, 500);
         }, assignHandlers = () => {
            this.draw_g.selectAll('.axis_zoom, .axis_labels')
                       .on('mousedown', startRectSel)
                       .on('dblclick', () => fp.unzoom('z'));

            if (settings.ZoomWheel) {
               this.draw_g.on('wheel', evnt => {
                  evnt.stopPropagation();
                  evnt.preventDefault();

                  const pos = d3_pointer(evnt, this.draw_g.node()),
                      coord = vertical ? (1 - pos[1] / palette_height) : pos[0] / palette_width,

                   item = fp.z_handle.analyzeWheelEvent(evnt, coord);
                  if (item.changed)
                     fp.zoomSingle('z', item.min, item.max);
               });
            }
         };

         fp.z_handle.setAfterDrawHandler(assignHandlers);

         assignHandlers();
      });
   }

   /** @summary draw RPalette object */
   static async draw(dom, palette, opt) {
      const painter = new RPalettePainter(dom, palette, opt, 'palette');
      return ensureRCanvas(painter, false).then(() => {
         painter.createG(); // just create container, real drawing will be done by histogram
         return painter;
      });
   }

} // class RPalettePainter

export { RPalettePainter, drawText, drawLine, drawBox, drawMarker };
