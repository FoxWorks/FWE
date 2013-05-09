varying float f_Depth;
varying vec4 v_Color;
varying vec4 v_Position;
uniform vec4 v_ClipPlane;

void main(void) {
  if (dot(v_ClipPlane,v_Position) < 0.0) {
    discard;
  } else {
    gl_FragColor = vec4(mod(v_Color.g/64.0,1.0),v_Color.r,f_Depth,1);
  }
}
