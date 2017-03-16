/**
 * Allpix
 * Author: John Idarraga <idarraga@cern.ch> , 2010
 Distance units in mm !
 */

#include "GeoDsc.h"



GeoDsc::GeoDsc() :
  m_ID(0),
  m_npix_x(0),
  m_npix_y(0),
  m_npix_z(0),
  m_pixsize_x(0),
  m_pixsize_y(0),
  m_pixsize_z(0),
  m_sensor_hx(0),
  m_sensor_hy(0),
  m_sensor_hz(0),
  m_coverlayer_hz(0),
  m_coverlayer_mat("Al"),
  m_coverlayer_ON(false),
  m_sensor_posx(0.),
  m_sensor_posy(0.),
  m_sensor_posz(0.),
  m_sensor_gr_excess_htop(0.),
  m_sensor_gr_excess_hbottom(0.),
  m_sensor_gr_excess_hright(0.),
  m_sensor_gr_excess_hleft(0.),
  m_chip_hx(0.),
  m_chip_hy(0.),
  m_chip_hz(0.),
  m_chip_offsetx(0.),
  m_chip_offsety(0.),
  m_chip_offsetz(0.),
  m_chip_posx(0.),
  m_chip_posy(0.),
  m_chip_posz(0.),
  m_pcb_hx(0.),
  m_pcb_hy(0.),
  m_pcb_hz(0.),
  m_bump_radius(0.),
  m_bump_height(0.),
  m_bump_offsetx(0.),
  m_bump_offsety(0.),
  m_bump_dr(0.),
  m_efieldfromfile(false)
{
}

GeoDsc::~GeoDsc() {

}

/* temporary ... this has to be changed by a db com or something like that */

void GeoDsc::Dump() {

  cout << "Dump geo description for object with Id : " << m_ID << endl;

  cout << "   Digitizer         : " << m_digitizer << endl;
  cout << "   npix_x            = " << m_npix_x << endl;
  cout << "   npix_y            = " << m_npix_y << endl;
  cout << "   npix_z            = " << m_npix_z << endl;
  cout << "   pixsize_x         = " << m_pixsize_x << " [mm]" << endl;
  cout << "   pixsize_y         = " << m_pixsize_y << endl;
  cout << "   pixsize_z         = " << m_pixsize_z << endl;
  cout << "   sensor_hx         = " << m_sensor_hx << ", posx "<< m_sensor_posx << endl;
  cout << "   sensor_hy         = " << m_sensor_hy << ", posy "<< m_sensor_posy << endl;
  cout << "   sensor_hz         = " << m_sensor_hz << ", posz "<< m_sensor_posz << endl;
  cout << "   coverlayer_hz     = " << m_coverlayer_hz << endl;
  cout << "   coverlayer_mat    = " << m_coverlayer_mat << endl;
  cout << "   chip_hx           = " << m_chip_hx << ", posx "<< m_chip_posx << endl;
  cout << "   chip_hy           = " << m_chip_hy << ", posy "<< m_chip_posy << endl;
  cout << "   chip_hz           = " << m_chip_hz << ", posz "<< m_chip_posz << endl;
  cout << "   pcb_hx            = " << m_pcb_hx << endl;
  cout << "   pcb_hy            = " << m_pcb_hy << endl;
  cout << "   pcb_hz            = " << m_pcb_hz << endl;

}

void GeoDsc::SetEFieldMap(TString valS){
  m_EFieldFile = valS;

  // Get EFieldFile from $allpix/valS
  struct stat buffer;
  if(!(stat (valS, &buffer))){
		
    m_efieldfromfile = true;

    ifstream efieldinput;
    efieldinput.open(valS);

    int nptsx, nptsy, nptsz;
    int currx, curry, currz;

    double ex, ey, ez;

    efieldinput >> nptsx >> nptsy >> nptsz;

    vector<TVector3> onedim;
    vector<vector<TVector3>> twodim;
    onedim.reserve(nptsx);
    twodim.reserve(nptsy);

    for (int k = 0; k < nptsz; k++) {
      for (int j = 0; j < nptsy; j++) {
	for (int i = 0; i < nptsx; i++) {
	  efieldinput >> currx >> curry >> currz;
	  efieldinput >> ex >> ey >> ez;
	  onedim.push_back(TVector3(ex, ey, ez));
	}
	twodim.push_back(onedim);
	onedim.clear();
      }
      m_efieldmap.push_back(twodim);
      twodim.clear();
    }

    m_efieldmap_nx = nptsx;
    m_efieldmap_ny = nptsy;
    m_efieldmap_nz = nptsz;

    efieldinput.close();

  }else{
    if(valS.Length())
      {
	cout << "File for EField named, but not found: " << valS << endl;
	cout << "This cannot end well... Abort." << endl;
	m_efieldfromfile = false;
	exit(1);
      }
    cout << "Found no EFieldFile " << valS << endl;
    m_efieldfromfile = false;
  }

}

inline TVector3 linear(const TVector3 value0, const TVector3 value1, const double p){

  TVector3 result;

  for (size_t i = 0; i < 3; i++) {
    result[i] = (value0[i] + (value1[i] - value0[i])*(p));
  }

  return result;
}

inline TVector3 bilinear(const TVector3* ecube, const double x, const double y, const int z){

  TVector3 result;

  TVector3 bil_y1 = linear(ecube[0+2*0+4*z], ecube[1+2*0+4*z], x);
  TVector3 bil_y2 = linear(ecube[0+2*1+4*z], ecube[1+2*1+4*z], x);

  return linear(bil_y1, bil_y2, y);

}

inline TVector3 trilinear(const TVector3* ecube, const TVector3 pos){

  TVector3 bil_z0, bil_z1;
  bil_z0 = bilinear(ecube, pos[0], pos[1], 0);
  bil_z1 = bilinear(ecube, pos[0], pos[1], 1);

  return linear(bil_z0, bil_z1, pos[2]);

}

inline int int_floor(const double x)
{
  int i = (int)x;
  return i - ( i > x );
}

inline double fmod2(const double x, const double y)
{
  return x-y*int_floor(x/y);
}

TVector3 GeoDsc::GetEFieldFromMap(TVector3 ppos){

  TVector3 currentefield;
	
  double pixsize_x = GetPixelX();
  double pixsize_y = GetPixelY();
  double pixsize_z = GetPixelZ();
	
  // ppos is the position in mm inside one pixel cell
	
  ppos = TVector3(fmod2(ppos[0],pixsize_x), fmod2(ppos[1],pixsize_y), ppos[2]);
	
  // Assuming that point 1 and nx are basically at the same position. The "-1" takes account of the efieldmap starting at the iterator 1.
  // Get the position iside the grid

  TVector3 pposgrid;
  pposgrid[0] = ppos[0]/pixsize_x*(double)(m_efieldmap_nx-1);
  pposgrid[1] = ppos[1]/pixsize_y*(double)(m_efieldmap_ny-1);
  pposgrid[2] = ppos[2]/pixsize_z*(double)(m_efieldmap_nz-1);

  // Got the position in units of the map coordinates. Do a 3D interpolation of the electric field.
  // Get the eight neighbors and do three linear interpolations.

  TVector3 * ecube = new TVector3[8];

  for (size_t i = 0; i < 2; i++) {
    for (size_t j = 0; j < 2; j++) {
      for (size_t k = 0; k < 2; k++) {
	ecube[i+2*j+4*k] = m_efieldmap.at(int_floor(pposgrid[2]+k)).at(int_floor(pposgrid[1]+j)).at(int_floor(pposgrid[0]+i));
      }
    }
  }

  // Make pposgrid the position inside the cube

  for (size_t i = 0; i < 3; i++) {
    pposgrid[i] -= (double)(int_floor(pposgrid[i]));
  }
	
  currentefield = trilinear(ecube, pposgrid);
  // Try to just weight the eight edges with the 3D distance to the sampling point.

  delete[] ecube;

  return currentefield;

}

