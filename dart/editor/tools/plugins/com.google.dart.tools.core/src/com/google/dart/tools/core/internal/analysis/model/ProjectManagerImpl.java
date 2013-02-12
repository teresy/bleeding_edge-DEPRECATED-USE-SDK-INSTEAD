/*
 * Copyright 2013 Dart project authors.
 * 
 * Licensed under the Eclipse Public License v1.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 * 
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.google.dart.tools.core.internal.analysis.model;

import com.google.dart.engine.index.Index;
import com.google.dart.engine.index.IndexFactory;
import com.google.dart.engine.search.SearchEngine;
import com.google.dart.engine.search.SearchEngineFactory;
import com.google.dart.engine.source.Source;
import com.google.dart.tools.core.analysis.model.Project;
import com.google.dart.tools.core.analysis.model.ProjectManager;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IWorkspaceRoot;

import java.util.HashMap;

/**
 * Concrete implementation of {@link ProjectManager}
 */
public class ProjectManagerImpl implements ProjectManager {

  private final IWorkspaceRoot resource;
  private final HashMap<IProject, Project> projects = new HashMap<IProject, Project>();
  private final Index index = IndexFactory.newIndex(IndexFactory.newMemoryIndexStore());

  public ProjectManagerImpl(IWorkspaceRoot resource) {
    this.resource = resource;
  }

  @Override
  public Index getIndex() {
    return index;
  }

  @Override
  public Project getProject(IProject resource) {
    Project result = projects.get(resource);
    if (result == null) {
      result = new ProjectImpl(resource);
      projects.put(resource, result);
    }
    return result;
  }

  @Override
  public Project[] getProjects() {
    IProject[] childResources = resource.getProjects();
    Project[] result = new Project[childResources.length];
    for (int index = 0; index < result.length; index++) {
      result[index] = getProject(childResources[index]);
    }
    return result;
  }

  @Override
  public IWorkspaceRoot getResource() {
    return resource;
  }

  @Override
  public IResource getResourceFor(Source source) {
    // TODO (danrubel): revisit and optimize performance
    for (Project project : getProjects()) {
      IResource res = project.getResourceFor(source);
      if (res != null) {
        return res;
      }
    }
    return null;
  }

  @Override
  public SearchEngine newSearchEngine() {
    return SearchEngineFactory.createSearchEngine(getIndex());
  }
}
