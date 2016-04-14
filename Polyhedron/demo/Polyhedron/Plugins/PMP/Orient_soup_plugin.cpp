#include <QApplication>
#include <QAction>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QtDebug>

#include "Scene_polygon_soup_item.h"
#include "Scene_polyhedron_item.h"
#include "Scene_surface_mesh_item.h"

#include <CGAL/Three/Polyhedron_demo_plugin_interface.h>
#include "Messages_interface.h"
using namespace CGAL::Three;
class Polyhedron_demo_orient_soup_plugin : 
  public QObject,
  public Polyhedron_demo_plugin_interface
{
  Q_OBJECT
  Q_INTERFACES(CGAL::Three::Polyhedron_demo_plugin_interface)
  Q_PLUGIN_METADATA(IID "com.geometryfactory.PolyhedronDemo.PluginInterface/1.0")

public:
  void init(QMainWindow* mainWindow,
            CGAL::Three::Scene_interface* scene_interface,
            Messages_interface* m);
            bool applicable(QAction* action) const {
    Q_FOREACH(CGAL::Three::Scene_interface::Item_id index, scene->selectionIndices()) {
      if(qobject_cast<Scene_polygon_soup_item*>(scene->item(index)))
        return true;
      else
        if (action==actionShuffle && 
            qobject_cast<Scene_polyhedron_item*>(scene->item(index)))
            return true;
    }
    return false;
  }

  QList<QAction*> actions() const;

public Q_SLOTS:
  void orient();
  void shuffle();
  void displayNonManifoldEdges();

private:
  CGAL::Three::Scene_interface* scene;
  Messages_interface* messages;
  QMainWindow* mw;
  QAction* actionOrient;
  QAction* actionShuffle;
  QAction* actionDisplayNonManifoldEdges;

}; // end Polyhedron_demo_orient_soup_plugin

void Polyhedron_demo_orient_soup_plugin::init(QMainWindow* mainWindow,
                                              CGAL::Three::Scene_interface* scene_interface,
                                              Messages_interface* m)
{
  scene = scene_interface;
  mw = mainWindow;
  messages = m;
  actionOrient = new QAction(tr("&Orient Polygon Soup"), mainWindow);
  actionOrient->setObjectName("actionOrient");
  actionOrient->setProperty("subMenuName", "Polygon Mesh Processing");
  connect(actionOrient, SIGNAL(triggered()),
          this, SLOT(orient()));

  actionShuffle = new QAction(tr("&Shuffle Polygon Soup"), mainWindow);
  actionShuffle->setProperty("subMenuName", "Polygon Mesh Processing");
  connect(actionShuffle, SIGNAL(triggered()),
          this, SLOT(shuffle()));

  actionDisplayNonManifoldEdges = new QAction(tr("Display Non Manifold Edges"),
                                              mainWindow);
  actionDisplayNonManifoldEdges->setProperty("subMenuName", "View");
  connect(actionDisplayNonManifoldEdges, SIGNAL(triggered()),
          this, SLOT(displayNonManifoldEdges()));
}

QList<QAction*> Polyhedron_demo_orient_soup_plugin::actions() const {
  return QList<QAction*>() << actionOrient
                           << actionShuffle
                           << actionDisplayNonManifoldEdges;
}

void set_vcolors(Scene_surface_mesh_item::SMesh* smesh, std::vector<CGAL::Color> colors)
{
  typedef Scene_surface_mesh_item::SMesh SMesh;
  typedef boost::graph_traits<SMesh>::vertex_descriptor vertex_descriptor;
  SMesh::Property_map<vertex_descriptor, CGAL::Color> vcolors =
    smesh->property_map<vertex_descriptor, CGAL::Color >("v:color").first;
  bool created;
  boost::tie(vcolors, created) = smesh->template add_property_map<SMesh::Vertex_index,CGAL::Color>("v:color",CGAL::Color(0,0,0));
  assert(colors.size()==smesh->number_of_vertices());
  int color_id = 0;
  BOOST_FOREACH(vertex_descriptor vd, vertices(*smesh))
      vcolors[vd] = colors[color_id++];
}

void set_fcolors(Scene_surface_mesh_item::SMesh* smesh, std::vector<CGAL::Color> colors)
{
  typedef Scene_surface_mesh_item::SMesh SMesh;
  typedef boost::graph_traits<SMesh>::face_descriptor face_descriptor;
  SMesh::Property_map<face_descriptor, CGAL::Color> fcolors =
    smesh->property_map<face_descriptor, CGAL::Color >("f:color").first;
  bool created;
   boost::tie(fcolors, created) = smesh->template add_property_map<SMesh::Face_index,CGAL::Color>("f:color",CGAL::Color(0,0,0));
  assert(colors.size()==smesh->number_of_faces());
  int color_id = 0;
  BOOST_FOREACH(face_descriptor fd, faces(*smesh))
      fcolors[fd] = colors[color_id++];
}

void Polyhedron_demo_orient_soup_plugin::orient()
{
  Q_FOREACH(CGAL::Three::Scene_interface::Item_id index, scene->selectionIndices())
  {
    Scene_polygon_soup_item* item = 
      qobject_cast<Scene_polygon_soup_item*>(scene->item(index));

    if(item)
    {
      //     qDebug()  << tr("I have the item %1\n").arg(item->name());
      QApplication::setOverrideCursor(Qt::WaitCursor);
      if(!item->orient()) {
         QMessageBox::information(mw, tr("Not orientable without self-intersections"),
                                      tr("The polygon soup \"%1\" is not directly orientable."
                                         " Some vertices have been duplicated and some self-intersections"
                                         " have been created.")
                                      .arg(item->name()));
      }

      if(!item->isDataColored())
      {
        Scene_polyhedron_item* poly_item = new Scene_polyhedron_item();
        if(item->exportAsPolyhedron(poly_item->polyhedron())) {
          poly_item->setName(item->name());
          poly_item->setColor(item->color());
          poly_item->setRenderingMode(item->renderingMode());
          poly_item->setVisible(item->visible());
          poly_item->invalidateOpenGLBuffers();
          poly_item->setProperty("source filename", item->property("source filename"));
          scene->replaceItem(index, poly_item);
          delete item;
        } else {
          item->invalidateOpenGLBuffers();
          scene->itemChanged(item);
        }
      }
      else
      {
        Scene_surface_mesh_item::SMesh* smesh = new Scene_surface_mesh_item::SMesh();
        if(item->exportAsSurfaceMesh(smesh)) {
          if(!item->getVColors().empty())
            set_vcolors(smesh,item->getVColors());
          if(!item->getFColors().empty())
            set_fcolors(smesh,item->getFColors());
          Scene_surface_mesh_item* sm_item = new Scene_surface_mesh_item(smesh);
          sm_item->setName(item->name());
          sm_item->setRenderingMode(item->renderingMode());
          sm_item->setVisible(item->visible());
          sm_item->setProperty("source filename", item->property("source filename"));
          scene->replaceItem(index, sm_item);
          delete item;
        } else {
          item->invalidateOpenGLBuffers();
          scene->itemChanged(item);
        }
      }

      QApplication::restoreOverrideCursor();
    }
    else{
      messages->warning(tr("This function is only applicable on polygon soups."));
    }
  }
}

void Polyhedron_demo_orient_soup_plugin::shuffle()
{
  const CGAL::Three::Scene_interface::Item_id index = scene->mainSelectionIndex();
  
  Scene_polygon_soup_item* item = 
    qobject_cast<Scene_polygon_soup_item*>(scene->item(index));

  if(item) {
    item->shuffle_orientations();
    //scene->itemChanged(item);
  }
  else {
    Scene_polyhedron_item* poly_item = 
      qobject_cast<Scene_polyhedron_item*>(scene->item(index));
    if(poly_item) {
      item = new Scene_polygon_soup_item();
      item->setName(poly_item->name());
      item->setRenderingMode(poly_item->renderingMode());
      item->setVisible(poly_item->visible());
      item->setProperty("source filename", poly_item->property("source filename"));
      item->load(poly_item);
      item->shuffle_orientations();
      item->setColor(poly_item->color());
      scene->replaceItem(index, item);
      delete poly_item;
    }
  }
}

void Polyhedron_demo_orient_soup_plugin::displayNonManifoldEdges()
{
  const CGAL::Three::Scene_interface::Item_id index = scene->mainSelectionIndex();
  
  Scene_polygon_soup_item* item = 
    qobject_cast<Scene_polygon_soup_item*>(scene->item(index));

  if(item)
  {
    item->setDisplayNonManifoldEdges(!item->displayNonManifoldEdges());
    scene->itemChanged(item);
  }
}

#include "Orient_soup_plugin.moc"
