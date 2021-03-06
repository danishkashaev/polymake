/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.ui.viewerapp.actions.view;

import java.awt.Component;
import java.awt.Dimension;
import java.awt.Frame;
import java.awt.event.ActionEvent;

import de.jreality.ui.viewerapp.actions.AbstractJrAction;
import de.jtem.beans.DimensionDialog;


/**
 * Allows to set the viewer size via a dimension panel.
 * 
 * @author msommer
 */
public class SetViewerSize extends AbstractJrAction {
	
	private Component viewer;

	public SetViewerSize(String name, Component viewer, Frame parentComp) {
		super(name, parentComp);

		if (viewer == null) 
			throw new IllegalArgumentException("Viewer not allowed to be null!");
		this.viewer = viewer;

		setShortDescription("Set the viewer size.");
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		Dimension d = viewer.getSize();
		Dimension dim = DimensionDialog.selectDimension(d, parentComp);
		if (dim == null) return;  //dialog cancelled
		
		viewer.setPreferredSize(null);  //need change of preferredSize (PropertyChangeEvent)
		viewer.setPreferredSize(dim);
		
		if (parentComp != null) ((Frame)parentComp).pack();
		viewer.requestFocusInWindow();
	}
	
}