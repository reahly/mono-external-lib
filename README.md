# mono external lib

Example Usage:
```c

auto trader_tooltip = mono::find_class( "Assembly-CSharp", "EFT.UI.TraderTooltip" );

const auto nickname_field = trader_tooltip->find_field( "_nickname" );
printf( "[nickname] -> %p : 0x%i", nickname_field, nickname_field->offset( ) );

const auto show_method = trader_tooltip->find_method( "Show" );
printf( "[Show] -> %p", show_method );

```
