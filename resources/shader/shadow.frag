uniform sampler2D s_Data;
varying vec2 v_texCoord2D;
uniform vec2 v_invScreenSize;

const float f_BlurMagnitude = 8.0;

void main(void) {
  vec4 result = vec4(0,0,0,0);

  result += texture2D(s_Data, vec2(v_texCoord2D.x - 1.00*f_BlurMagnitude*v_invScreenSize.x, v_texCoord2D.y)) * 0.05;
  result += texture2D(s_Data, vec2(v_texCoord2D.x - 0.75*f_BlurMagnitude*v_invScreenSize.x, v_texCoord2D.y)) * 0.09;
  result += texture2D(s_Data, vec2(v_texCoord2D.x - 0.50*f_BlurMagnitude*v_invScreenSize.x, v_texCoord2D.y)) * 0.12;
  result += texture2D(s_Data, vec2(v_texCoord2D.x - 0.25*f_BlurMagnitude*v_invScreenSize.x, v_texCoord2D.y)) * 0.15;
  result += texture2D(s_Data, vec2(v_texCoord2D.x + 0.00*f_BlurMagnitude*v_invScreenSize.x, v_texCoord2D.y)) * 0.16;
  result += texture2D(s_Data, vec2(v_texCoord2D.x + 0.25*f_BlurMagnitude*v_invScreenSize.x, v_texCoord2D.y)) * 0.15;
  result += texture2D(s_Data, vec2(v_texCoord2D.x + 0.50*f_BlurMagnitude*v_invScreenSize.x, v_texCoord2D.y)) * 0.12;
  result += texture2D(s_Data, vec2(v_texCoord2D.x + 0.75*f_BlurMagnitude*v_invScreenSize.x, v_texCoord2D.y)) * 0.09;
  result += texture2D(s_Data, vec2(v_texCoord2D.x + 1.00*f_BlurMagnitude*v_invScreenSize.x, v_texCoord2D.y)) * 0.05;

  result += texture2D(s_Data, vec2(v_texCoord2D.x, v_texCoord2D.y - 1.00*f_BlurMagnitude*v_invScreenSize.y)) * 0.05;
  result += texture2D(s_Data, vec2(v_texCoord2D.x, v_texCoord2D.y - 0.75*f_BlurMagnitude*v_invScreenSize.y)) * 0.09;
  result += texture2D(s_Data, vec2(v_texCoord2D.x, v_texCoord2D.y - 0.50*f_BlurMagnitude*v_invScreenSize.y)) * 0.12;
  result += texture2D(s_Data, vec2(v_texCoord2D.x, v_texCoord2D.y - 0.25*f_BlurMagnitude*v_invScreenSize.y)) * 0.15;
  result += texture2D(s_Data, vec2(v_texCoord2D.x, v_texCoord2D.y + 0.00*f_BlurMagnitude*v_invScreenSize.y)) * 0.16;
  result += texture2D(s_Data, vec2(v_texCoord2D.x, v_texCoord2D.y + 0.25*f_BlurMagnitude*v_invScreenSize.y)) * 0.15;
  result += texture2D(s_Data, vec2(v_texCoord2D.x, v_texCoord2D.y + 0.50*f_BlurMagnitude*v_invScreenSize.y)) * 0.12;
  result += texture2D(s_Data, vec2(v_texCoord2D.x, v_texCoord2D.y + 0.75*f_BlurMagnitude*v_invScreenSize.y)) * 0.09;
  result += texture2D(s_Data, vec2(v_texCoord2D.x, v_texCoord2D.y + 1.00*f_BlurMagnitude*v_invScreenSize.y)) * 0.05;
  
  result += texture2D(s_Data, vec2(
    v_texCoord2D.x + 0.50*f_BlurMagnitude*v_invScreenSize.x,
    v_texCoord2D.y + 0.50*f_BlurMagnitude*v_invScreenSize.y)) * 0.10;
  result += texture2D(s_Data, vec2(
    v_texCoord2D.x + 0.50*f_BlurMagnitude*v_invScreenSize.x,
    v_texCoord2D.y - 0.50*f_BlurMagnitude*v_invScreenSize.y)) * 0.10;
  result += texture2D(s_Data, vec2(
    v_texCoord2D.x - 0.50*f_BlurMagnitude*v_invScreenSize.x,
    v_texCoord2D.y - 0.50*f_BlurMagnitude*v_invScreenSize.y)) * 0.10;
  result += texture2D(s_Data, vec2(
    v_texCoord2D.x - 0.50*f_BlurMagnitude*v_invScreenSize.x,
    v_texCoord2D.y + 0.50*f_BlurMagnitude*v_invScreenSize.y)) * 0.10;

  gl_FragColor = vec4(0.0,0.0,0.0,result.a*0.40);
  
//  gl_FragColor = result;
//  gl_FragColor = vec4(texture2D(s_Data,v_texCoord2D).r,0.0,0.0,1.0);
}
