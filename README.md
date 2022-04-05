# mono external lib

Example Usage:
```c

static auto trader_tooltip = mono::find_class( "Assembly-CSharp", "EFT.UI.TraderTooltip" );

const auto nickname_field = trader_tooltip->find_field( "_nickname" );
printf( "[nickname] -> %p : 0x%i", nickname_field, nickname_field->offset( ) );

const auto show_method = trader_tooltip->find_method( "Show" );
printf( "[Show] -> %p", show_method );

static auto eft_hard_settings = mono::find_class( "Assembly-CSharp", "EFTHardSettings" )->get_vtable( mono::get_root_domain( ) )->get_static_field_data( );
printf( "[EFTHardSettings] -> %p", eft_hard_settings );
```
