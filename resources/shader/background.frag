varying vec2 v_texCoord2D;
uniform vec3 v_baseColor;

void main(void) {
  float tx = v_texCoord2D.x;
  float ty = 1.0-v_texCoord2D.y;
  
  //Base gradient
  vec3 total_color = v_baseColor;
  
  //Highlights and shadows
  total_color = total_color + 90.0*pow(1.0-ty,0.5)*pow(tx,1.0);
  total_color = total_color - 110.0*pow(ty,1.0);
  total_color = total_color - 110.0*pow(1.0-ty,2.0)*pow(1.0-tx,4.0);
  
  gl_FragColor = vec4(total_color/255.0,1.0);
}
