uniform sampler2D s_Data;
varying vec2 v_texCoord2D;
uniform vec2 v_invScreenSize;
uniform float f_outlineThickness;

const vec3 v_selectionColor = vec3(1.0,0.7,0.0);

void main(void) {
  //Get color of nearby points
  vec4 c00 = texture2D(s_Data, vec2(v_texCoord2D.x-v_invScreenSize.x*f_outlineThickness,v_texCoord2D.y));
  vec4 c01 = texture2D(s_Data, vec2(v_texCoord2D.x+v_invScreenSize.x*f_outlineThickness,v_texCoord2D.y));
  vec4 c10 = texture2D(s_Data, vec2(v_texCoord2D.x,v_texCoord2D.y-v_invScreenSize.y*f_outlineThickness));
  vec4 c11 = texture2D(s_Data, vec2(v_texCoord2D.x,v_texCoord2D.y+v_invScreenSize.y*f_outlineThickness));

  //Contour shader
  float dx = c00.r - c01.r; //Cross-section based contour
  float dy = c10.r - c11.r;
  float d = sqrt(dx*dx+dy*dy);
  
  float kx = c00.b - c01.b; //Depth-based contour
  float ky = c10.b - c11.b;
  float k = sqrt(kx*kx+ky*ky);

  float contour = 0.0;
  if ((d > 0.5/64.0) || (k > 0.01)) {
    contour = 1.0;
  }
  
  //Get color for the contour
  float objectSelected = min(1.0,c00.g + c01.g + c10.g + c11.g);
  
  gl_FragColor = vec4(v_selectionColor*objectSelected,contour);
//  gl_FragColor = texture2D(s_Data,v_texCoord2D);
}
